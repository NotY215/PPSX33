using System.Drawing.Drawing2D;

namespace PS3Recomp.Gui.Ui;

/// <summary>Glass panel with a liquid pill tab strip. Pages are plain controls (Dock=Fill).</summary>
internal sealed class GlassTabs : GlassPanel
{
    readonly List<(string Title, Control Page)> _pages = new();
    readonly System.Windows.Forms.Timer _slide = new() { Interval = 16 };
    RectangleF _cur, _target;
    int _sel = -1, _hot = -1;

    public GlassTabs()
    {
        _slide.Tick += (_, _) =>
        {
            _cur = new RectangleF(
                _cur.X + (_target.X - _cur.X) * 0.28f, _cur.Y + (_target.Y - _cur.Y) * 0.28f,
                _cur.Width + (_target.Width - _cur.Width) * 0.28f, _cur.Height + (_target.Height - _cur.Height) * 0.28f);
            if (Math.Abs(_cur.X - _target.X) < 0.4f && Math.Abs(_cur.Width - _target.Width) < 0.4f) { _cur = _target; _slide.Stop(); }
            Invalidate(HeaderBounds);
        };
    }

    protected override void Dispose(bool disposing)
    {
        if (disposing) _slide.Dispose();
        base.Dispose(disposing);
    }

    Font TabFont => Theme.Semi(9.25f);
    int HeaderH => Dpi.S(this, 40);
    Rectangle HeaderBounds => new(0, 0, Width, ShadowPx + HeaderH + Dpi.S(this, 14));

    public void AddPage(string title, Control page)
    {
        page.Dock = DockStyle.Fill;
        page.Visible = _pages.Count == 0;
        Controls.Add(page);
        _pages.Add((title, page));
        SetInset(12, 14 + 40 + 6, 12, 12);
        if (_sel < 0) { _sel = 0; _cur = _target = PillRect(0); }
        Invalidate();
    }

    RectangleF PillRect(int index)
    {
        float x = ShadowPx + Dpi.S(this, 16);
        float y = ShadowPx + Dpi.S(this, 12);
        using var f = TabFont;
        for (int i = 0; i < _pages.Count; i++)
        {
            float w = TextRenderer.MeasureText(_pages[i].Title, f).Width + Dpi.S(this, 28);
            if (i == index) return new RectangleF(x, y, w, Dpi.S(this, 30));
            x += w + Dpi.S(this, 6);
        }
        return RectangleF.Empty;
    }

    int Hit(Point p)
    {
        for (int i = 0; i < _pages.Count; i++) if (PillRect(i).Contains(p)) return i;
        return -1;
    }

    void Select(int i)
    {
        if (i < 0 || i == _sel) return;
        _sel = i;
        for (int k = 0; k < _pages.Count; k++) _pages[k].Page.Visible = k == i;
        _target = PillRect(i);
        _slide.Start();
        Invalidate();
    }

    protected override void OnMouseMove(MouseEventArgs e)
    {
        int h = Hit(e.Location);
        if (h != _hot) { _hot = h; Cursor = h >= 0 ? Cursors.Hand : Cursors.Default; Invalidate(HeaderBounds); }
        base.OnMouseMove(e);
    }
    protected override void OnMouseLeave(EventArgs e) { _hot = -1; Cursor = Cursors.Default; Invalidate(HeaderBounds); base.OnMouseLeave(e); }
    protected override void OnMouseDown(MouseEventArgs e) { Select(Hit(e.Location)); base.OnMouseDown(e); }

    protected override void OnPaint(PaintEventArgs e)
    {
        base.OnPaint(e);
        if (_pages.Count == 0) return;
        var g = e.Graphics;
        Gfx.HQ(g);

        // separator under the strip
        float sy = ShadowPx + Dpi.S(this, 52);
        using (var sp = new Pen(Color.FromArgb(28, 190, 210, 255), 1f))
            g.DrawLine(sp, ShadowPx + Dpi.S(this, 14), sy, Width - ShadowPx - Dpi.S(this, 14), sy);

        // liquid selection pill
        if (_sel >= 0 && !_cur.IsEmpty)
        {
            using var pp = Gfx.Round(_cur, _cur.Height / 2f);
            using (var glow = new Pen(Color.FromArgb(34, Theme.Cyan), 6f)) g.DrawPath(glow, pp);
            using (var fb = new LinearGradientBrush(_cur, Color.FromArgb(120, Theme.Blue), Color.FromArgb(110, Theme.Violet), 0f))
                g.FillPath(fb, pp);
            var st = g.Save();
            g.SetClip(pp, CombineMode.Intersect);
            var gl = new RectangleF(_cur.X, _cur.Y, _cur.Width, _cur.Height * 0.5f);
            using (var gb = new LinearGradientBrush(gl, Color.FromArgb(70, 255, 255, 255), Color.FromArgb(0, 255, 255, 255), 90f))
                g.FillRectangle(gb, gl);
            g.Restore(st);
            using var pen = new Pen(Color.FromArgb(150, 255, 255, 255), 1f);
            g.DrawPath(pen, pp);
        }

        using var f = TabFont;
        for (int i = 0; i < _pages.Count; i++)
        {
            var pr = PillRect(i);
            Color c = i == _sel ? Color.White : (i == _hot ? Theme.TextPri : Theme.TextSec);
            TextRenderer.DrawText(g, _pages[i].Title, f, Rectangle.Round(pr), c,
                TextFormatFlags.HorizontalCenter | TextFormatFlags.VerticalCenter | TextFormatFlags.NoPadding | TextFormatFlags.SingleLine);
        }
    }
}

/// <summary>Top bar: emblem built from the four face-button symbols, gradient wordmark and pipeline chips.</summary>
internal sealed class HeaderBar : GlassPanel
{
    float _pulse;
    readonly System.Windows.Forms.Timer _t = new() { Interval = 50 };

    public HeaderBar()
    {
        _t.Tick += (_, _) => { _pulse += 0.09f; Invalidate(new Rectangle(Width - Dpi.S(this, 190), 0, Dpi.S(this, 190), Height)); };
        _t.Start();
    }

    protected override void Dispose(bool disposing)
    {
        if (disposing) _t.Dispose();
        base.Dispose(disposing);
    }

    protected override void OnPaint(PaintEventArgs e)
    {
        base.OnPaint(e);
        var g = e.Graphics;
        Gfx.HQ(g);
        var r = GlassRect;
        float k = Dpi.K(this);

        // emblem
        float es = r.Height - 20 * k;
        var em = new RectangleF(r.X + 16 * k, r.Y + (r.Height - es) / 2f, es, es);
        using (var ep = Gfx.Round(em, es * 0.30f))
        {
            using var fb = new LinearGradientBrush(em, Color.FromArgb(120, Theme.Blue), Color.FromArgb(120, Theme.Violet), 45f);
            g.FillPath(fb, ep);
            using var pen = new Pen(Color.FromArgb(170, 255, 255, 255), 1.1f);
            g.DrawPath(pen, ep);
        }
        float q = es * 0.215f, cx = em.X + es / 2f, cy = em.Y + es / 2f, d = es * 0.25f;
        Gfx.Glyph(g, 0, new RectangleF(cx - q, cy - d - q, q * 2, q * 2), Theme.PsTriangle, 1.8f);
        Gfx.Glyph(g, 1, new RectangleF(cx + d - q, cy - q, q * 2, q * 2), Theme.PsCircle, 1.8f);
        Gfx.Glyph(g, 2, new RectangleF(cx - q, cy + d - q, q * 2, q * 2), Theme.PsCross, 1.8f);
        Gfx.Glyph(g, 3, new RectangleF(cx - d - q, cy - q, q * 2, q * 2), Theme.PsSquare, 1.8f);

        // wordmark
        float tx = em.Right + 14 * k;
        using (var f = new Font("Segoe UI Semibold", 18f, FontStyle.Regular, GraphicsUnit.Point))
        {
            var sz = g.MeasureString("PPSX33", f);
            var tr = new RectangleF(tx, r.Y + 6 * k, sz.Width, sz.Height);
            using var gb = new LinearGradientBrush(tr, Theme.TextPri, Theme.Cyan, 0f);
            g.DrawString("PPSX33", f, gb, tr.Location);
        }
        using (var f = Theme.Semi(7.5f))
        using (var b = new SolidBrush(Theme.TextSec))
            g.DrawString("STATIC RECOMPILER   ·   PPU  /  SPU  /  RSX", f, b, tx + 1, r.Y + r.Height - 24 * k);

        // chips (right aligned)
        string[] chips = { "CELL B.E.  PPC64 BE", "SELF → ELF → C++ → x64" };
        float x = r.Right - 16 * k;
        using var cf = Theme.Semi(8f);

        // live engine dot
        string live = "ENGINE READY";
        var ls = TextRenderer.MeasureText(live, cf);
        float lw = ls.Width + 28 * k, ch = 26 * k, cyy = r.Y + (r.Height - ch) / 2f;
        x -= lw;
        var lr = new RectangleF(x, cyy, lw, ch);
        using (var lp = Gfx.Round(lr, ch / 2f))
        {
            using var fb = new SolidBrush(Color.FromArgb(26, Theme.Success));
            g.FillPath(fb, lp);
            using var pen = new Pen(Color.FromArgb(90, Theme.Success), 1f);
            g.DrawPath(pen, lp);
        }
        float pr = 3.2f * k + MathF.Sin(_pulse) * 0.9f * k;
        using (var db = new SolidBrush(Theme.Success))
            g.FillEllipse(db, lr.X + 12 * k - pr, lr.Y + ch / 2f - pr, pr * 2, pr * 2);
        TextRenderer.DrawText(g, live, cf, new Rectangle((int)(lr.X + 20 * k), (int)lr.Y, (int)(lr.Width - 20 * k), (int)ch), Theme.Success,
            TextFormatFlags.Left | TextFormatFlags.VerticalCenter | TextFormatFlags.NoPadding);
        x -= 10 * k;

        for (int i = chips.Length - 1; i >= 0; i--)
        {
            var s = TextRenderer.MeasureText(chips[i], cf);
            float w = s.Width + 24 * k;
            x -= w;
            if (x < tx + 220 * k) break;      // not enough room: drop remaining chips
            var cr = new RectangleF(x, cyy, w, ch);
            using (var cp = Gfx.Round(cr, ch / 2f))
            {
                using var fb = new SolidBrush(Color.FromArgb(16, 200, 220, 255));
                g.FillPath(fb, cp);
                using var pen = new Pen(Color.FromArgb(46, 200, 220, 255), 1f);
                g.DrawPath(pen, cp);
            }
            TextRenderer.DrawText(g, chips[i], cf, Rectangle.Round(cr), Theme.TextSec,
                TextFormatFlags.HorizontalCenter | TextFormatFlags.VerticalCenter | TextFormatFlags.NoPadding);
            x -= 8 * k;
        }
    }
}
