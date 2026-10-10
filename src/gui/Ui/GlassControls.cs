using System.Drawing.Drawing2D;

namespace PS3Recomp.Gui.Ui;

internal enum GlassKind { Step, Primary, Danger }

/// <summary>Liquid-glass button: hover blob follows the cursor, press squish, click ripple.</summary>
internal sealed class GlassButton : Button
{
    public GlassKind Kind { get; set; } = GlassKind.Step;
    public int Glyph { get; set; } = -1;

    readonly System.Windows.Forms.Timer _anim = new() { Interval = 16 };
    float _hover, _press, _rippleT = 1f;
    bool _over, _down;
    PointF _mouse, _ripple;

    static readonly Color[] GlyphColors =
    {
        Theme.PsTriangle, Theme.PsCircle, Theme.PsCross, Theme.PsSquare, Theme.Cyan, Theme.TextSec, Theme.Danger
    };

    public GlassButton()
    {
        SetStyle(ControlStyles.UserPaint | ControlStyles.OptimizedDoubleBuffer | ControlStyles.ResizeRedraw |
                 ControlStyles.SupportsTransparentBackColor | ControlStyles.Selectable, true);
        BackColor = Color.Transparent;
        FlatStyle = FlatStyle.Flat;
        FlatAppearance.BorderSize = 0;
        UseVisualStyleBackColor = false;
        Cursor = Cursors.Hand;
        Font = Theme.Semi(9.5f);
        _anim.Tick += (_, _) => Tick();
    }

    protected override void Dispose(bool disposing)
    {
        if (disposing) _anim.Dispose();
        base.Dispose(disposing);
    }

    void Tick()
    {
        float th = _over && Enabled ? 1f : 0f, tp = _down && Enabled ? 1f : 0f;
        _hover += (th - _hover) * 0.22f;
        _press += (tp - _press) * 0.35f;
        if (_rippleT < 1f) _rippleT = Math.Min(1f, _rippleT + 0.045f);
        if (Math.Abs(th - _hover) < 0.01f && Math.Abs(tp - _press) < 0.01f && _rippleT >= 1f)
        {
            _hover = th; _press = tp; _anim.Stop();
        }
        Invalidate();
    }

    void Kick() { if (!_anim.Enabled) _anim.Start(); }

    protected override void OnMouseEnter(EventArgs e) { _over = true; Kick(); base.OnMouseEnter(e); }
    protected override void OnMouseLeave(EventArgs e) { _over = false; _down = false; Kick(); base.OnMouseLeave(e); }
    protected override void OnMouseMove(MouseEventArgs e) { _mouse = e.Location; if (_over) Invalidate(); base.OnMouseMove(e); }
    protected override void OnMouseDown(MouseEventArgs e)
    {
        _down = true; _ripple = e.Location; _rippleT = 0f; Kick(); base.OnMouseDown(e);
    }
    protected override void OnMouseUp(MouseEventArgs e) { _down = false; Kick(); base.OnMouseUp(e); }
    protected override void OnEnabledChanged(EventArgs e) { _over = _down = false; Kick(); Invalidate(); base.OnEnabledChanged(e); }
    protected override void OnGotFocus(EventArgs e) { Invalidate(); base.OnGotFocus(e); }
    protected override void OnLostFocus(EventArgs e) { Invalidate(); base.OnLostFocus(e); }

    protected override void OnPaint(PaintEventArgs e)
    {
        var g = e.Graphics;
        Gfx.HQ(g);
        float k = Enabled ? 1f : 0.42f;
        float sq = _press * 1.6f;
        var r = new RectangleF(1.5f + sq, 1.5f + sq, Width - 3f - sq * 2, Height - 3f - sq * 2);
        float rad = Math.Min(Dpi.S(this, 14), r.Height / 2f);
        using var path = Gfx.Round(r, rad);

        // body
        switch (Kind)
        {
            case GlassKind.Primary:
                using (var br = new LinearGradientBrush(r,
                    Color.FromArgb((int)(225 * k), Gfx.Mix(Theme.Blue, Theme.Cyan, _hover * 0.55f)),
                    Color.FromArgb((int)(205 * k), Gfx.Mix(Theme.Violet, Theme.Blue, _hover * 0.55f)), 0f))
                    g.FillPath(br, path);
                break;
            case GlassKind.Danger:
                using (var br = new SolidBrush(Color.FromArgb((int)((38 + 46 * _hover) * k), Theme.Danger)))
                    g.FillPath(br, path);
                break;
            default:
                using (var br = new SolidBrush(Color.FromArgb((int)((16 + 26 * _hover) * k), 200, 220, 255)))
                    g.FillPath(br, path);
                break;
        }

        // gloss, cursor blob, ripple (clipped to the shape)
        var st = g.Save();
        g.SetClip(path, CombineMode.Intersect);
        var gloss = new RectangleF(r.X, r.Y, r.Width, r.Height * 0.55f);
        using (var gb = new LinearGradientBrush(gloss, Color.FromArgb((int)((Kind == GlassKind.Primary ? 80 : 46) * k), 255, 255, 255), Color.FromArgb(0, 255, 255, 255), 90f))
            g.FillRectangle(gb, gloss);

        if (_hover > 0.02f && Enabled)
        {
            float br = Math.Max(Width, 90) * 0.55f;
            using var bp = new GraphicsPath();
            bp.AddEllipse(_mouse.X - br, _mouse.Y - br, br * 2, br * 2);
            using var pg = new PathGradientBrush(bp)
            {
                CenterColor = Color.FromArgb((int)(70 * _hover), 255, 255, 255),
                SurroundColors = new[] { Color.FromArgb(0, 255, 255, 255) },
                CenterPoint = _mouse
            };
            g.FillPath(pg, bp);
        }
        if (_rippleT < 1f)
        {
            float rr = _rippleT * Width * 1.1f;
            using var rb = new SolidBrush(Color.FromArgb((int)((1f - _rippleT) * 90), 255, 255, 255));
            g.FillEllipse(rb, _ripple.X - rr, _ripple.Y - rr, rr * 2, rr * 2);
        }
        g.Restore(st);

        // rim
        Color rimC = Kind == GlassKind.Danger ? Gfx.A(Theme.Danger, (int)((120 + 60 * _hover) * k)) : Color.FromArgb((int)((Kind == GlassKind.Primary ? 130 : 70 + 70 * _hover) * k), 255, 255, 255);
        using (var pen = new Pen(rimC, 1f)) g.DrawPath(pen, path);
        if (Focused && ShowFocusCues)
        {
            using var fp = Gfx.Round(RectangleF.Inflate(r, -2.5f, -2.5f), Math.Max(2, rad - 2));
            using var pen = new Pen(Color.FromArgb(180, Theme.Cyan), 1.2f);
            g.DrawPath(pen, fp);
        }

        // glyph badge
        float badge = Dpi.S(this, 26);
        var br2 = new RectangleF(r.X + Dpi.S(this, 9), r.Y + (r.Height - badge) / 2f, badge, badge);
        if (Glyph >= 0)
        {
            using (var bb = new SolidBrush(Color.FromArgb((int)((Kind == GlassKind.Primary ? 38 : 20) * k), 255, 255, 255)))
                g.FillEllipse(bb, br2);
            Color gc = Kind == GlassKind.Primary ? Color.White : GlyphColors[Math.Clamp(Glyph, 0, GlyphColors.Length - 1)];
            var inner = RectangleF.Inflate(br2, -badge * 0.28f, -badge * 0.28f);
            Gfx.Glyph(g, Glyph, inner, Gfx.A(gc, (int)(255 * k)), 1.7f);
        }

        // label
        Color tc = Kind switch
        {
            GlassKind.Primary => Color.White,
            GlassKind.Danger => Color.FromArgb(255, 175, 185),
            _ => Theme.TextPri
        };
        tc = Gfx.A(tc, (int)(255 * (Enabled ? 1f : 0.5f)));
        float tx = Glyph >= 0 ? br2.Right + Dpi.S(this, 9) : r.X + Dpi.S(this, 14);
        var tr = new RectangleF(tx, r.Y, Math.Max(8, r.Right - tx - Dpi.S(this, 16)), r.Height);
        using (var sf = new StringFormat { LineAlignment = StringAlignment.Center, Trimming = StringTrimming.EllipsisCharacter, FormatFlags = StringFormatFlags.NoWrap })
        using (var tb = new SolidBrush(tc))
            g.DrawString(Text, Font, tb, tr, sf);

        // chevron (slides in on hover)
        if (_hover > 0.05f && Kind != GlassKind.Danger)
        {
            float cx2 = r.Right - Dpi.S(this, 16) + (1f - _hover) * 4f, cy = r.Y + r.Height / 2f;
            using var cp = new Pen(Color.FromArgb((int)(150 * _hover * k), 255, 255, 255), 1.6f) { StartCap = LineCap.Round, EndCap = LineCap.Round };
            g.DrawLines(cp, new[] { new PointF(cx2 - 2.5f, cy - 4f), new PointF(cx2 + 1.5f, cy), new PointF(cx2 - 2.5f, cy + 4f) });
        }
    }
}

/// <summary>Capsule progress indicator with a liquid glowing runner. API-compatible with the bits of ProgressBar the form uses.</summary>
internal sealed class GlassProgress : Control
{
    readonly System.Windows.Forms.Timer _t = new() { Interval = 33 };
    float _phase, _fade;

    public ProgressBarStyle Style { get; set; } = ProgressBarStyle.Marquee;
    public int MarqueeAnimationSpeed { get; set; }

    public GlassProgress()
    {
        SetStyle(ControlStyles.UserPaint | ControlStyles.OptimizedDoubleBuffer | ControlStyles.ResizeRedraw |
                 ControlStyles.SupportsTransparentBackColor, true);
        BackColor = Color.Transparent;
        _t.Tick += (_, _) =>
        {
            bool on = MarqueeAnimationSpeed > 0;
            _fade += ((on ? 1f : 0f) - _fade) * 0.14f;
            if (on) _phase += 0.018f;
            if (on || _fade > 0.01f) Invalidate();
        };
        _t.Start();
    }

    protected override void Dispose(bool disposing)
    {
        if (disposing) _t.Dispose();
        base.Dispose(disposing);
    }

    protected override void OnPaint(PaintEventArgs e)
    {
        var g = e.Graphics;
        Gfx.HQ(g);
        float h = Dpi.S(this, 8);
        var track = new RectangleF(2, (Height - h) / 2f, Math.Max(10, Width - 4), h);
        using var tp = Gfx.Round(track, h / 2f);
        using (var tb = new SolidBrush(Color.FromArgb(26, 200, 220, 255))) g.FillPath(tb, tp);
        using (var bp = new Pen(Color.FromArgb(46, 200, 220, 255), 1f)) g.DrawPath(bp, tp);
        if (_fade < 0.02f) return;

        var st = g.Save();
        g.SetClip(tp, CombineMode.Intersect);
        float w = track.Width * 0.38f;
        float u = (_phase % 1.5f) - 0.25f;
        float x = track.X + (track.Width + w) * u / 1.0f - w * 0.3f;
        var seg = new RectangleF(x, track.Y, w, track.Height);
        int a = (int)(255 * _fade);
        using (var glow = new Pen(Color.FromArgb(a / 4, Theme.Cyan), h * 1.8f))
            g.DrawLine(glow, seg.X + 4, track.Y + h / 2f, seg.Right - 4, track.Y + h / 2f);
        using (var sp = Gfx.Round(seg, h / 2f))
        using (var sb = new LinearGradientBrush(new RectangleF(seg.X - 1, seg.Y, seg.Width + 2, seg.Height), Color.FromArgb(0, Theme.Cyan), Color.FromArgb(a, Theme.Violet), 0f))
        {
            sb.InterpolationColors = new ColorBlend(3)
            {
                Positions = new[] { 0f, 0.55f, 1f },
                Colors = new[] { Color.FromArgb(0, Theme.Cyan), Color.FromArgb(a, Theme.Cyan), Color.FromArgb(a, Theme.Violet) }
            };
            g.FillPath(sb, sp);
        }
        g.Restore(st);
    }
}
