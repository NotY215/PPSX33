using System.Drawing.Drawing2D;
using System.Drawing.Imaging;

namespace PS3Recomp.Gui.Ui;

internal static class Dpi
{
    public static float K(Control c) => Math.Max(1f, c.DeviceDpi / 96f);
    public static int S(Control c, float v) => (int)Math.Round(v * K(c));
}

/// <summary>
/// "Liquid glass" panel: frosted blur of the animated backdrop, tint, specular highlight, bright rim and a
/// soft floating shadow (spatial depth). Child controls sit on the glass.
/// The panel's outer ring (Shadow) is reserved for the drop shadow, so neighbours can be docked flush.
/// </summary>
internal class GlassPanel : Panel
{
    public float Radius { get; set; } = 18f;
    public Color Tint { get; set; } = Theme.Ink;
    public int TintAlpha { get; set; } = 105;
    protected LiquidBackdrop? Backdrop;
    /// <summary>Optional extra drawing on top of the glass (icons etc.).</summary>
    public Action<Graphics, RectangleF>? Decorate { get; set; }

    public GlassPanel()
    {
        SetStyle(ControlStyles.UserPaint | ControlStyles.AllPaintingInWmPaint | ControlStyles.OptimizedDoubleBuffer |
                 ControlStyles.ResizeRedraw | ControlStyles.Opaque, true);
        BackColor = Theme.Ink;
        ForeColor = Theme.TextPri;
    }

    public int ShadowPx => Dpi.S(this, 8);

    /// <summary>Sets padding relative to the glass edge (shadow ring is added automatically).</summary>
    public void SetInset(int l, int t, int r, int b)
    {
        int s = ShadowPx;
        Padding = new Padding(s + Dpi.S(this, l), s + Dpi.S(this, t), s + Dpi.S(this, r), s + Dpi.S(this, b));
    }

    public RectangleF GlassRect
    {
        get
        {
            int s = ShadowPx;
            return new RectangleF(s, s, Math.Max(2, Width - s * 2), Math.Max(2, Height - s * 2));
        }
    }

    // ------------------------------------------------------------------ backdrop hookup
    LiquidBackdrop? FindBackdrop()
    {
        for (Control? c = Parent; c != null; c = c.Parent)
            if (c is LiquidBackdrop b) return b;
        return null;
    }

    void Hook()
    {
        var b = FindBackdrop();
        if (ReferenceEquals(b, Backdrop)) return;
        if (Backdrop != null) Backdrop.Frame -= OnFrame;
        Backdrop = b;
        if (Backdrop != null) Backdrop.Frame += OnFrame;
    }

    protected override void OnParentChanged(EventArgs e) { base.OnParentChanged(e); Hook(); }
    protected override void OnHandleCreated(EventArgs e) { base.OnHandleCreated(e); Hook(); }
    protected override void Dispose(bool disposing)
    {
        if (disposing && Backdrop != null) { Backdrop.Frame -= OnFrame; Backdrop = null; }
        base.Dispose(disposing);
    }

    void OnFrame()
    {
        if (IsHandleCreated && Visible && Width > 0 && Height > 0) Invalidate(true);
    }

    // ------------------------------------------------------------------ paint
    protected override void OnPaintBackground(PaintEventArgs e) { }

    protected override void OnPaint(PaintEventArgs e)
    {
        var g = e.Graphics;
        Hook();
        var bd = Backdrop;

        // 1) crisp scene under the panel (shows through the shadow ring)
        if (bd != null)
        {
            var o = bd.ToBackdrop(this);
            var st = g.Save();
            g.TranslateTransform(-o.X, -o.Y);
            bd.DrawScene(g, new Rectangle(e.ClipRectangle.X + o.X, e.ClipRectangle.Y + o.Y, e.ClipRectangle.Width, e.ClipRectangle.Height));
            g.Restore(st);
        }
        else
        {
            using var br = new SolidBrush(Theme.Void);
            g.FillRectangle(br, e.ClipRectangle);
        }

        Gfx.HQ(g);
        var r = GlassRect;
        float rad = Dpi.S(this, Radius);

        // 2) floating shadow
        int sp = ShadowPx;
        for (int i = sp; i >= 1; i--)
        {
            var rr = RectangleF.Inflate(r, i * 0.9f, i * 0.9f);
            rr.Offset(0, sp * 0.28f);
            using var p = Gfx.Round(rr, rad + i * 0.9f);
            using var b = new SolidBrush(Color.FromArgb(7, 0, 0, 12));
            g.FillPath(b, p);
        }

        using var path = Gfx.Round(r, rad);
        var state = g.Save();
        g.SetClip(path, CombineMode.Intersect);

        // 3) frosted blur of the backdrop
        var low = bd?.Blurred;
        if (bd != null && low != null)
        {
            var o = bd.ToBackdrop(this);
            float sc = LiquidBackdrop.BlurScale;
            var src = new RectangleF((o.X + r.X) / sc, (o.Y + r.Y) / sc, r.Width / sc, r.Height / sc);
            using var ia = new ImageAttributes();
            ia.SetWrapMode(WrapMode.TileFlipXY);
            g.PixelOffsetMode = PixelOffsetMode.Half;
            g.DrawImage(low, Rectangle.Round(r), src.X, src.Y, src.Width, src.Height, GraphicsUnit.Pixel, ia);
            g.PixelOffsetMode = PixelOffsetMode.Default;
        }

        // 4) tint (keeps text legible) + glass body gradient
        using (var tb = new SolidBrush(Color.FromArgb(TintAlpha, Tint))) g.FillRectangle(tb, r);
        using (var body = new LinearGradientBrush(r, Color.FromArgb(34, 190, 215, 255), Color.FromArgb(6, 120, 150, 255), 90f))
            g.FillRectangle(body, r);

        // 5) drifting sheen (moves with the mouse parallax -> liquid feel)
        float cx = 0.5f + (bd?.ParallaxX ?? 0f) * 0.38f;
        var sheen = new LinearGradientBrush(r, Color.Transparent, Color.Transparent, 18f);
        sheen.InterpolationColors = new ColorBlend(5)
        {
            Positions = new[] { 0f, Math.Clamp(cx - 0.20f, 0.01f, 0.97f), Math.Clamp(cx, 0.02f, 0.98f), Math.Clamp(cx + 0.20f, 0.03f, 0.99f), 1f },
            Colors = new[] { Color.FromArgb(0, 255, 255, 255), Color.FromArgb(0, 255, 255, 255), Color.FromArgb(20, 220, 235, 255), Color.FromArgb(0, 255, 255, 255), Color.FromArgb(0, 255, 255, 255) }
        };
        g.FillRectangle(sheen, r);
        sheen.Dispose();

        // 6) top specular lens
        float hh = Math.Min(r.Height * 0.55f, Dpi.S(this, 120));
        var spec = new RectangleF(r.X - r.Width * 0.1f, r.Y - hh * 0.9f, r.Width * 1.2f, hh * 1.55f);
        using (var sp2 = new GraphicsPath())
        {
            sp2.AddEllipse(spec);
            using var pb = new PathGradientBrush(sp2)
            {
                CenterColor = Color.FromArgb(0, 255, 255, 255),
                SurroundColors = new[] { Color.FromArgb(38, 255, 255, 255) },
                CenterPoint = new PointF(spec.X + spec.Width / 2, spec.Bottom)
            };
            g.FillRectangle(pb, r.X, r.Y, r.Width, hh);
        }

        // 7) inner liquid rim (thick, soft) – gives the "glass thickness"
        using (var rim = new Pen(Color.FromArgb(22, 200, 225, 255), Dpi.S(this, 6)) { LineJoin = LineJoin.Round })
            g.DrawPath(rim, path);
        g.Restore(state);

        // 8) crisp rim light: bright top-left, cyan bottom-right
        using (var edge = new LinearGradientBrush(r, Color.FromArgb(200, 255, 255, 255), Color.FromArgb(110, Theme.Cyan), 55f))
        {
            var cb = new ColorBlend(4)
            {
                Positions = new[] { 0f, 0.25f, 0.7f, 1f },
                Colors = new[] { Color.FromArgb(190, 255, 255, 255), Color.FromArgb(40, 190, 210, 255), Color.FromArgb(34, 150, 180, 255), Color.FromArgb(120, Theme.Cyan) }
            };
            edge.InterpolationColors = cb;
            using var pen = new Pen(edge, 1.2f);
            g.DrawPath(pen, path);
        }

        Decorate?.Invoke(g, r);
    }
}

/// <summary>Rounded inset "well" used to frame consoles and inputs on top of glass.</summary>
internal sealed class FieldFrame : Panel
{
    public Color Fill { get; set; } = Color.FromArgb(9, 13, 28);
    public float Radius { get; set; } = 12f;
    public bool FillChild { get; set; }
    bool _focus;

    public FieldFrame()
    {
        SetStyle(ControlStyles.UserPaint | ControlStyles.OptimizedDoubleBuffer | ControlStyles.ResizeRedraw |
                 ControlStyles.SupportsTransparentBackColor, true);
        BackColor = Color.Transparent;
    }

    protected override void OnControlAdded(ControlEventArgs e)
    {
        base.OnControlAdded(e);
        if (e.Control != null)
        {
            e.Control.GotFocus += (_, _) => { _focus = true; Invalidate(); };
            e.Control.LostFocus += (_, _) => { _focus = false; Invalidate(); };
        }
    }

    protected override void OnLayout(LayoutEventArgs levent)
    {
        base.OnLayout(levent);
        foreach (Control c in Controls)
        {
            if (FillChild) c.SetBounds(Padding.Left, Padding.Top, Math.Max(1, Width - Padding.Horizontal), Math.Max(1, Height - Padding.Vertical));
            else c.SetBounds(Padding.Left, Math.Max(0, (Height - c.Height) / 2), Math.Max(1, Width - Padding.Horizontal), c.Height);
        }
    }

    protected override void OnPaint(PaintEventArgs e)
    {
        var g = e.Graphics;
        Gfx.HQ(g);
        var r = new RectangleF(0.5f, 0.5f, Width - 1.5f, Height - 1.5f);
        using var p = Gfx.Round(r, Dpi.S(this, Radius));
        using (var b = new SolidBrush(Fill)) g.FillPath(b, p);
        using var pen = _focus
            ? new Pen(Color.FromArgb(200, Theme.Cyan), 1.4f)
            : new Pen(Color.FromArgb(46, 190, 210, 255), 1f);
        g.DrawPath(pen, p);
    }
}
