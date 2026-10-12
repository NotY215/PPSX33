using System.Drawing.Drawing2D;

namespace PS3Recomp.Gui;

/// <summary>Opaque, double-buffered PS3 XMB-inspired background. No transparency or glass effects.</summary>
internal sealed class Ps3BackdropPanel : Panel
{
    private Bitmap? _backgroundCache;
    private readonly System.Windows.Forms.Timer _resizePaintTimer;
    private int _stableResizeTicks;

    public Ps3BackdropPanel()
    {
        DoubleBuffered = true;
        SetStyle(ControlStyles.UserPaint | ControlStyles.AllPaintingInWmPaint |
                 ControlStyles.OptimizedDoubleBuffer | ControlStyles.ResizeRedraw, true);
        BackColor = Color.FromArgb(5, 8, 20);
        // Rebuild at a steady ~30 Hz during live resizing, then stop after the size settles.
        _resizePaintTimer = new System.Windows.Forms.Timer { Interval = 33 };
        _resizePaintTimer.Tick += (_, _) =>
        {
            if (ClientSize.Width <= 0 || ClientSize.Height <= 0) return;
            if (_backgroundCache == null || _backgroundCache.Width != ClientSize.Width || _backgroundCache.Height != ClientSize.Height)
            {
                RebuildBackground();
                _stableResizeTicks = 0;
                Invalidate();
            }
            else if (++_stableResizeTicks >= 4)
            {
                _resizePaintTimer.Stop();
            }
        };
        SizeChanged += (_, _) =>
        {
            _stableResizeTicks = 0;
            if (!_resizePaintTimer.Enabled) _resizePaintTimer.Start();
            Invalidate();
        };
        Disposed += (_, _) => { _resizePaintTimer.Stop(); _resizePaintTimer.Dispose(); _backgroundCache?.Dispose(); };
    }

    protected override void OnPaintBackground(PaintEventArgs e)
    {
        if (ClientSize.Width <= 0 || ClientSize.Height <= 0) return;
        if (_backgroundCache == null) {
            RebuildBackground();
        }
        if (_backgroundCache != null)
            e.Graphics.DrawImage(_backgroundCache, ClientRectangle);
        else
            e.Graphics.Clear(BackColor);
    }

    private void RebuildBackground()
    {
        if (ClientSize.Width <= 0 || ClientSize.Height <= 0) return;
        var next = new Bitmap(ClientSize.Width, ClientSize.Height);
        using (var g = Graphics.FromImage(next)) {
            g.SmoothingMode = SmoothingMode.HighQuality;
            var r = new Rectangle(Point.Empty, ClientSize);
            using (var bg = new LinearGradientBrush(r, Color.FromArgb(5, 8, 20), Color.FromArgb(10, 28, 63), 35f))
                g.FillRectangle(bg, r);
            DrawWave(g, r, 0.58f, 0.075f, 0.9f, Color.FromArgb(70, 80, 195, 255), 2.0f);
            DrawWave(g, r, 0.68f, 0.060f, 1.3f, Color.FromArgb(48, 50, 130, 255), 3.0f);
            DrawWave(g, r, 0.77f, 0.045f, 1.7f, Color.FromArgb(34, 105, 110, 255), 4.0f);
            using var glowPath = new GraphicsPath();
            glowPath.AddEllipse(-r.Width * .2f, r.Height * .02f, r.Width * .75f, r.Height * .75f);
            using var glow = new PathGradientBrush(glowPath) {
                CenterColor = Color.FromArgb(25, 70, 160, 255),
                SurroundColors = new[] { Color.FromArgb(0, 30, 80, 190) }
            };
            g.FillRectangle(glow, r);
            using var vignette = new LinearGradientBrush(r, Color.FromArgb(0, 0, 0, 0), Color.FromArgb(45, 0, 2, 12), 90f);
            g.FillRectangle(vignette, r);
        }
        var previous = _backgroundCache;
        _backgroundCache = next;
        previous?.Dispose();
    }

    static void DrawWave(Graphics g, Rectangle r, float baseY, float amplitude, float frequency, Color color, float phase)
    {
        int count = Math.Clamp(r.Width / 14, 40, 180);
        var pts = new PointF[count + 2];
        for (int i = 0; i < count; i++) {
            float u = i / (float)(count - 1);
            float x = r.Left + u * r.Width;
            float y = r.Top + r.Height * baseY
                + MathF.Sin(u * MathF.Tau * frequency + phase) * r.Height * amplitude
                + MathF.Sin(u * MathF.Tau * frequency * 1.7f - phase) * r.Height * amplitude * .3f;
            pts[i] = new PointF(x, y);
        }
        pts[count] = new PointF(r.Right, r.Bottom);
        pts[count + 1] = new PointF(r.Left, r.Bottom);
        using var brush = new LinearGradientBrush(r, color, Color.FromArgb(0, color), 90f);
        g.FillPolygon(brush, pts);
        using var pen = new Pen(Color.FromArgb(Math.Min(125, color.A + 35), 80, 190, 255), 1.2f);
        g.DrawLines(pen, pts.Take(count).ToArray());
    }
}

/// <summary>Custom-painted WinForms button with PS3-inspired blue focus and readable disabled states.</summary>
internal sealed class Ps3Button : Button
{
    bool _hover, _down;
    bool _glowPulse;
    readonly System.Windows.Forms.Timer _glowTimer;

    public Ps3Button()
    {
        _glowTimer = new System.Windows.Forms.Timer { Interval = 90 };
        _glowTimer.Tick += (_, _) => { _glowPulse = !_glowPulse; Invalidate(); };
        Disposed += (_, _) => { _glowTimer.Stop(); _glowTimer.Dispose(); };
        FlatStyle = FlatStyle.Flat;
        FlatAppearance.BorderSize = 0;
        UseVisualStyleBackColor = false;
        SetStyle(ControlStyles.UserPaint | ControlStyles.AllPaintingInWmPaint |
                 ControlStyles.OptimizedDoubleBuffer | ControlStyles.ResizeRedraw, true);
        Font = new Font("Segoe UI Semibold", 9.25f);
        ForeColor = Color.FromArgb(238, 244, 255);
        Cursor = Cursors.Hand;
        TabStop = true;
    }

    protected override void OnMouseEnter(EventArgs e) { _hover = true; _glowTimer.Start(); Invalidate(); base.OnMouseEnter(e); }
    protected override void OnMouseLeave(EventArgs e) { _hover = false; _down = false; _glowTimer.Stop(); _glowPulse = false; Invalidate(); base.OnMouseLeave(e); }
    protected override void OnGotFocus(EventArgs e) { base.OnGotFocus(e); if (!_glowTimer.Enabled) _glowTimer.Start(); Invalidate(); }
    protected override void OnLostFocus(EventArgs e) { base.OnLostFocus(e); if (!_hover) { _glowTimer.Stop(); _glowPulse = false; } Invalidate(); }
    protected override void OnMouseDown(MouseEventArgs e) { if (e.Button == MouseButtons.Left && Enabled) _down = true; Invalidate(); base.OnMouseDown(e); }
    protected override void OnMouseUp(MouseEventArgs e) { _down = false; Invalidate(); base.OnMouseUp(e); }
    protected override void OnEnabledChanged(EventArgs e) { base.OnEnabledChanged(e); if (!Enabled) { _glowTimer.Stop(); _glowPulse = false; } Invalidate(); }

    protected override void OnPaint(PaintEventArgs e)
    {
        var g = e.Graphics;
        g.SmoothingMode = SmoothingMode.AntiAlias;
        var rect = new RectangleF(1, 1, Math.Max(1, Width - 3), Math.Max(1, Height - 3));
        Color baseColor = Enabled ? BackColor : Color.FromArgb(21, 30, 53);
        Color top = !_down && _hover && Enabled ? Color.FromArgb(65, 145, 245) : baseColor;
        Color bottom = Enabled ? ControlPaint.Dark(baseColor, .28f) : Color.FromArgb(15, 22, 40);
        using (var path = Rounded(rect, 5))
        using (var brush = new LinearGradientBrush(rect, top, bottom, LinearGradientMode.Vertical)) {
            g.FillPath(brush, path);
            using var border = new Pen(_hover && Enabled ? Color.FromArgb(145, 210, 255) : Color.FromArgb(55, 105, 175), _hover ? 1.8f : 1f);
            g.DrawPath(border, path);
        }
        if (Enabled && (Focused || _hover)) {
            using var glowPath = Rounded(new RectangleF(2, 2, Math.Max(1, Width - 5), Math.Max(1, Height - 5)), 5);
            int glowAlpha = _hover ? (_glowPulse ? 145 : 75) : 50;
            float glowWidth = _hover ? (_glowPulse ? 4.0f : 2.8f) : 1.8f;
            using var glow = new Pen(Color.FromArgb(glowAlpha, 35, 165, 255), glowWidth);
            g.DrawPath(glow, glowPath);
            using var shine = new Pen(Color.FromArgb(_hover ? 85 : 35, 135, 215, 255), 1f);
            g.DrawLine(shine, 8, 3, Math.Max(8, Width - 9), 3);
        }
        var textColor = Enabled ? ForeColor : Color.FromArgb(125, 142, 175);
        TextRenderer.DrawText(g, Text, Font, Rectangle.Round(rect), textColor,
            TextFormatFlags.HorizontalCenter | TextFormatFlags.VerticalCenter | TextFormatFlags.EndEllipsis | TextFormatFlags.SingleLine);
        if (ShowFocusCues && Focused) {
            var focus = Rectangle.Round(rect);
            focus.Inflate(-5, -5);
            ControlPaint.DrawFocusRectangle(g, focus, textColor, Color.Transparent);
        }
    }

    static GraphicsPath Rounded(RectangleF r, float radius)
    {
        var p = new GraphicsPath();
        float d = Math.Min(radius * 2, Math.Min(r.Width, r.Height));
        p.AddArc(r.X, r.Y, d, d, 180, 90);
        p.AddArc(r.Right - d, r.Y, d, d, 270, 90);
        p.AddArc(r.Right - d, r.Bottom - d, d, d, 0, 90);
        p.AddArc(r.X, r.Bottom - d, d, d, 90, 90);
        p.CloseFigure();
        return p;
    }
}

/// <summary>Compact opaque HUD card used for project and telemetry labels.</summary>
internal sealed class Ps3HudLabel : Label
{
    public Ps3HudLabel()
    {
        SetStyle(ControlStyles.UserPaint | ControlStyles.AllPaintingInWmPaint |
                 ControlStyles.OptimizedDoubleBuffer | ControlStyles.ResizeRedraw, true);
        BackColor = Color.FromArgb(12, 22, 47);
        ForeColor = Color.FromArgb(235, 244, 255);
        Padding = new Padding(12, 0, 8, 0);
    }

    protected override void OnPaint(PaintEventArgs e)
    {
        e.Graphics.SmoothingMode = SmoothingMode.AntiAlias;
        e.Graphics.Clear(BackColor);
        var r = ClientRectangle;
        using var border = new Pen(Color.FromArgb(55, 115, 190), 1f);
        e.Graphics.DrawRectangle(border, 0, 0, Math.Max(0, r.Width - 1), Math.Max(0, r.Height - 1));
        using var accent = new Pen(Color.FromArgb(65, 190, 255), 2f);
        e.Graphics.DrawLine(accent, 1, 1, 22, 1);
        e.Graphics.DrawLine(accent, 1, 1, 1, Math.Min(14, r.Height - 1));
        TextRenderer.DrawText(e.Graphics, Text, Font, new Rectangle(Padding.Left, 0,
            Math.Max(0, r.Width - Padding.Horizontal), r.Height), ForeColor,
            TextFormatFlags.Left | TextFormatFlags.VerticalCenter | TextFormatFlags.EndEllipsis | TextFormatFlags.SingleLine);
    }
}
