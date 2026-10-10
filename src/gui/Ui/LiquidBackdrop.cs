using System.Drawing.Drawing2D;
using System.Drawing.Imaging;

namespace PS3Recomp.Gui.Ui;

/// <summary>
/// Animated "XMB night" backdrop: drifting aurora orbs, flowing PS3-style wave ribbons and a faint
/// drifting disassembly / hex-dump layer. Reacts to the mouse (parallax) for a spatial feel.
/// A blurred low-resolution copy of the scene is shared with GlassPanel so panels can look frosted.
/// </summary>
internal sealed class LiquidBackdrop : Control
{
    public const int BlurScale = 6;

    readonly System.Windows.Forms.Timer _timer = new() { Interval = 50 };
    readonly DateTime _t0 = DateTime.UtcNow;
    Bitmap? _low;
    float _t;
    float _px, _py;               // smoothed parallax in -1..1
    PointF[] _waveA = Array.Empty<PointF>();
    PointF[] _waveB = Array.Empty<PointF>();
    PointF[] _waveC = Array.Empty<PointF>();
    Bitmap?[] _hexCols = Array.Empty<Bitmap?>();
    int _hexW = -1, _hexH = -1;

    public event Action? Frame;
    public Bitmap? Blurred => _low;
    public float ParallaxX => _px;
    public float ParallaxY => _py;

    static readonly string[] Asm =
    {
        "7F 45 4C 46 02 02 01 00", "mflr   r0", "stdu   r1,-0x90(r1)", "std    r31,0x88(r1)",
        "lis    r3,0x1000", "addi   r3,r3,0x4C", "bl     .sys_process_exit", "lwz    r9,0(r3)",
        "cmpwi  cr7,r9,0", "beq    cr7,loc_10234", "li     r11,0x3A", "sc",
        "0x00010000  38 60 00 01", "0x00010004  4E 80 00 20", "blr", "ld     r2,0x28(r1)",
        "SPU  ila  $3,0x1000", "SPU  shufb $4,$5,$6,$7", "RSX  FIFO  0x0000_4A10", "mtctr  r12",
        "bctrl", "ori    r0,r0,0", "rldicl r9,r9,0,32", "stw    r0,0x84(r1)", "SELF  NPDRM  AES-CTR",
        "PRX  import  cellGcmSys", "NID  0xB2E761D4", "lfd    f1,0x10(r3)", "fadds  f0,f1,f2",
    };

    public LiquidBackdrop()
    {
        SetStyle(ControlStyles.UserPaint | ControlStyles.AllPaintingInWmPaint | ControlStyles.OptimizedDoubleBuffer |
                 ControlStyles.ResizeRedraw | ControlStyles.Opaque, true);
        BackColor = Theme.Void;
        _timer.Tick += (_, _) => Step();
        _timer.Start();
    }

    protected override void Dispose(bool disposing)
    {
        if (disposing)
        {
            _timer.Dispose();
            _low?.Dispose();
            foreach (var b in _hexCols) b?.Dispose();
        }
        base.Dispose(disposing);
    }

    void Step()
    {
        var f = FindForm();
        if (!Visible || f == null || f.WindowState == FormWindowState.Minimized || Width < 8 || Height < 8) return;

        _t = (float)(DateTime.UtcNow - _t0).TotalSeconds;

        var mp = PointToClient(MousePosition);
        float tx = Math.Clamp((mp.X / (float)Width) * 2f - 1f, -1f, 1f);
        float ty = Math.Clamp((mp.Y / (float)Height) * 2f - 1f, -1f, 1f);
        _px += (tx - _px) * 0.07f;
        _py += (ty - _py) * 0.07f;

        RenderBlurSource();
        Frame?.Invoke();
        Invalidate();
    }

    // ---------------------------------------------------------------- scene (soft layer, also the blur source)
    void RenderBlurSource()
    {
        int lw = Math.Max(8, Width / BlurScale + 1), lh = Math.Max(8, Height / BlurScale + 1);
        if (_low == null || _low.Width != lw || _low.Height != lh)
        {
            _low?.Dispose();
            _low = new Bitmap(lw, lh, PixelFormat.Format32bppPArgb);
        }
        using var g = Graphics.FromImage(_low);
        g.SmoothingMode = SmoothingMode.AntiAlias;
        g.ScaleTransform(1f / BlurScale, 1f / BlurScale);
        DrawSoft(g, Width, Height);
    }

    void DrawSoft(Graphics g, int w, int h)
    {
        using (var bg = new LinearGradientBrush(new Rectangle(0, 0, w, h), Theme.Void, Theme.Ink, 62f))
            g.FillRectangle(bg, 0, 0, w, h);

        // aurora orbs (depth-dependent parallax)
        Orb(g, w, h, 0.18f, 0.22f, 0.55f, 0.20f, 0.13f, 0.6f, 1.0f, Theme.Blue, 150);
        Orb(g, w, h, 0.82f, 0.30f, 0.50f, 0.17f, 0.11f, 1.3f, 1.4f, Theme.Violet, 135);
        Orb(g, w, h, 0.55f, 0.85f, 0.60f, 0.22f, 0.10f, 2.1f, 0.7f, Theme.Cyan, 105);
        Orb(g, w, h, 0.10f, 0.88f, 0.38f, 0.12f, 0.15f, 3.4f, 1.8f, Theme.PsSquare, 70);
        Orb(g, w, h, 0.92f, 0.82f, 0.34f, 0.10f, 0.12f, 4.2f, 1.2f, Theme.PsTriangle, 60);

        // XMB ribbons
        Ribbon(g, ref _waveA, w, h, 0.60f, 0.070f, 0.9f, 0.7f, 0, Theme.Cyan, 78, 1.2f);
        Ribbon(g, ref _waveB, w, h, 0.68f, 0.060f, 1.3f, -0.55f, 2, Theme.Blue, 70, 1.8f);
        Ribbon(g, ref _waveC, w, h, 0.76f, 0.050f, 1.8f, 0.4f, 4, Theme.Violet, 62, 2.4f);
    }

    void Orb(Graphics g, int w, int h, float bx, float by, float size, float ax, float ay, float phase, float depth, Color c, int alpha)
    {
        float cx = w * (bx + ax * MathF.Sin(_t * 0.17f + phase)) + _px * 60f * depth;
        float cy = h * (by + ay * MathF.Cos(_t * 0.13f + phase * 1.7f)) + _py * 40f * depth;
        float r = Math.Max(w, h) * size;
        using var path = new GraphicsPath();
        path.AddEllipse(cx - r, cy - r, r * 2, r * 2);
        using var pg = new PathGradientBrush(path)
        {
            CenterColor = Color.FromArgb(alpha, c),
            SurroundColors = new[] { Color.FromArgb(0, c) },
            CenterPoint = new PointF(cx, cy)
        };
        g.FillPath(pg, path);
    }

    void Ribbon(Graphics g, ref PointF[] pts, int w, int h, float yBase, float amp, float speed, float dir, float phase, Color c, int alpha, float freq)
    {
        int n = Math.Max(8, w / 14);
        if (pts.Length != n + 3) pts = new PointF[n + 3];
        for (int i = 0; i <= n; i++)
        {
            float x = w * i / (float)n;
            float u = i / (float)n;
            float y = h * yBase
                + MathF.Sin(u * 6.28f * freq * 0.6f + _t * speed * dir + phase) * h * amp
                + MathF.Sin(u * 6.28f * freq * 1.3f - _t * speed * 0.6f + phase * 2f) * h * amp * 0.45f
                + _py * 26f + _px * 18f * (u - 0.5f);
            pts[i] = new PointF(x, y);
        }
        pts[n + 1] = new PointF(w, h);
        pts[n + 2] = new PointF(0, h);
        using var br = new LinearGradientBrush(new Rectangle(0, (int)(h * (yBase - amp * 1.5f)), Math.Max(1, w), Math.Max(1, (int)(h * (1.1f - yBase)))),
            Color.FromArgb(alpha, c), Color.FromArgb(0, c), 90f);
        g.FillPolygon(br, pts);
    }

    // ---------------------------------------------------------------- crisp layer
    protected override void OnPaintBackground(PaintEventArgs e) { }

    protected override void OnPaint(PaintEventArgs e) => DrawScene(e.Graphics, e.ClipRectangle);

    /// <summary>Paints the full crisp scene into g (g may be translated so this control's origin maps correctly).</summary>
    public void DrawScene(Graphics g, Rectangle clip)
    {
        var low = _low;
        if (low == null)
        {
            using var br = new SolidBrush(Theme.Void);
            g.FillRectangle(br, clip);
            return;
        }

        var old = g.Save();
        g.SetClip(clip);
        g.InterpolationMode = InterpolationMode.HighQualityBilinear;
        g.PixelOffsetMode = PixelOffsetMode.Half;
        using (var ia = new ImageAttributes())
        {
            ia.SetWrapMode(WrapMode.TileFlipXY);
            g.DrawImage(low, new Rectangle(0, 0, Width, Height), 0, 0, low.Width, low.Height, GraphicsUnit.Pixel, ia);
        }
        g.PixelOffsetMode = PixelOffsetMode.Default;

        // subtle perspective floor grid
        g.SmoothingMode = SmoothingMode.None;
        using (var gp = new Pen(Color.FromArgb(10, 160, 190, 255), 1f))
        {
            float ox = _px * -14f, oy = _py * -10f;
            for (float x = (ox % 56f); x < Width; x += 56f) g.DrawLine(gp, x, 0, x, Height);
            for (float y = (oy % 56f); y < Height; y += 56f) g.DrawLine(gp, 0, y, Width, y);
        }

        DrawHex(g);

        // crisp ribbon strokes (thin glowing lines over the soft fills)
        g.SmoothingMode = SmoothingMode.AntiAlias;
        StrokeWave(g, _waveA, Theme.Cyan, 95, 1.2f);
        StrokeWave(g, _waveB, Theme.Blue, 80, 1.1f);
        StrokeWave(g, _waveC, Theme.Violet, 70, 1.0f);

        // vignette
        using (var path = new GraphicsPath())
        {
            path.AddRectangle(new Rectangle(-Width / 3, -Height / 3, Width * 5 / 3, Height * 5 / 3));
            using var pg = new PathGradientBrush(path)
            {
                CenterColor = Color.FromArgb(0, 0, 0, 0),
                SurroundColors = new[] { Color.FromArgb(150, 2, 3, 10) },
                FocusScales = new PointF(0.62f, 0.62f)
            };
            g.FillRectangle(pg, 0, 0, Width, Height);
        }
        g.Restore(old);
    }

    static void StrokeWave(Graphics g, PointF[] pts, Color c, int alpha, float w)
    {
        if (pts.Length < 5) return;
        int n = pts.Length - 2;
        using var glow = new Pen(Color.FromArgb(alpha / 4, c), w * 5f) { LineJoin = LineJoin.Round };
        using var core = new Pen(Color.FromArgb(alpha, c), w) { LineJoin = LineJoin.Round };
        var line = new PointF[n];
        Array.Copy(pts, line, n);
        g.DrawCurve(glow, line, 0.35f);
        g.DrawCurve(core, line, 0.35f);
    }

    // drifting disassembly columns (pre-rendered tiles, only blitted per frame)
    void DrawHex(Graphics g)
    {
        if (_hexCols.Length == 0 || _hexW != Width || _hexH != Height) BuildHex();
        int colW = 210;
        float speed = 9f;
        for (int i = 0; i < _hexCols.Length; i++)
        {
            var tile = _hexCols[i];
            if (tile == null) continue;
            float dir = (i % 2 == 0) ? -1f : 1f;
            float off = ((_t * speed * (0.7f + 0.2f * (i % 3)) * dir) % tile.Height + tile.Height) % tile.Height;
            float x = 14 + i * colW + _px * -10f * (1 + i % 3 * 0.4f);
            for (float y = -off; y < Height; y += tile.Height)
                g.DrawImageUnscaled(tile, (int)x, (int)y);
        }
    }

    void BuildHex()
    {
        foreach (var b in _hexCols) b?.Dispose();
        _hexW = Width; _hexH = Height;
        int cols = Math.Max(1, Width / 210);
        _hexCols = new Bitmap?[cols];
        int lineH = 15;
        int lines = Math.Max(12, Height / lineH + 4);
        var rnd = new Random(33);
        using var font = Theme.Mono(8f);
        for (int i = 0; i < cols; i++)
        {
            var bmp = new Bitmap(200, lines * lineH, PixelFormat.Format32bppPArgb);
            using (var gg = Graphics.FromImage(bmp))
            {
                gg.TextRenderingHint = System.Drawing.Text.TextRenderingHint.AntiAliasGridFit;
                for (int l = 0; l < lines; l++)
                {
                    string s = Asm[rnd.Next(Asm.Length)];
                    bool hot = rnd.Next(11) == 0;
                    var c = hot ? Color.FromArgb(46, Theme.Cyan) : Color.FromArgb(20, 150, 175, 255);
                    using var br = new SolidBrush(c);
                    gg.DrawString(s, font, br, 0, l * lineH);
                }
            }
            _hexCols[i] = bmp;
        }
    }

    // ---------------------------------------------------------------- helpers
    /// <summary>Position of a child control's top-left expressed in backdrop coordinates.</summary>
    public Point ToBackdrop(Control c) => PointToClient(c.PointToScreen(Point.Empty));
}
