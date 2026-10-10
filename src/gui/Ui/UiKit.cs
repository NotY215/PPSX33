using System.Drawing.Drawing2D;
using System.Runtime.InteropServices;

namespace PS3Recomp.Gui.Ui;

/// <summary>Shared palette + drawing helpers for the glass / liquid UI. UI only.</summary>
internal static class Theme
{
    // Deep PS3 "XMB" night palette
    public static readonly Color Void      = Color.FromArgb(6, 8, 18);
    public static readonly Color Deep      = Color.FromArgb(10, 14, 32);
    public static readonly Color Ink       = Color.FromArgb(14, 20, 44);
    public static readonly Color TextPri   = Color.FromArgb(238, 242, 255);
    public static readonly Color TextSec   = Color.FromArgb(150, 162, 196);
    public static readonly Color TextDim   = Color.FromArgb(98, 110, 146);
    public static readonly Color Cyan      = Color.FromArgb(70, 200, 255);
    public static readonly Color Blue      = Color.FromArgb(70, 130, 255);
    public static readonly Color Violet    = Color.FromArgb(150, 100, 255);
    public static readonly Color Success   = Color.FromArgb(80, 220, 160);
    public static readonly Color Danger    = Color.FromArgb(255, 100, 120);
    public static readonly Color Console   = Color.FromArgb(9, 13, 28);

    // Face-button colours (triangle / circle / cross / square)
    public static readonly Color PsTriangle = Color.FromArgb(70, 220, 170);
    public static readonly Color PsCircle   = Color.FromArgb(255, 95, 110);
    public static readonly Color PsCross    = Color.FromArgb(90, 150, 255);
    public static readonly Color PsSquare   = Color.FromArgb(240, 130, 220);

    public static Font Font(float size, FontStyle style = FontStyle.Regular) =>
        new Font("Segoe UI", size, style, GraphicsUnit.Point);
    public static Font Semi(float size) => new Font("Segoe UI Semibold", size, FontStyle.Regular, GraphicsUnit.Point);
    public static Font Mono(float size) => new Font("Cascadia Mono", size, FontStyle.Regular, GraphicsUnit.Point) is { } f && f.Name == "Cascadia Mono"
        ? f : new Font("Consolas", size, FontStyle.Regular, GraphicsUnit.Point);
}

internal static class Gfx
{
    public static GraphicsPath Round(RectangleF r, float radius)
    {
        var p = new GraphicsPath();
        float d = Math.Min(radius * 2, Math.Min(r.Width, r.Height));
        if (d <= 0) { p.AddRectangle(r); return p; }
        p.AddArc(r.X, r.Y, d, d, 180, 90);
        p.AddArc(r.Right - d, r.Y, d, d, 270, 90);
        p.AddArc(r.Right - d, r.Bottom - d, d, d, 0, 90);
        p.AddArc(r.X, r.Bottom - d, d, d, 90, 90);
        p.CloseFigure();
        return p;
    }

    public static Color A(Color c, int alpha) => Color.FromArgb(Math.Clamp(alpha, 0, 255), c);

    public static Color Mix(Color a, Color b, float t)
    {
        t = Math.Clamp(t, 0f, 1f);
        return Color.FromArgb(
            (int)(a.A + (b.A - a.A) * t), (int)(a.R + (b.R - a.R) * t),
            (int)(a.G + (b.G - a.G) * t), (int)(a.B + (b.B - a.B) * t));
    }

    public static void HQ(Graphics g)
    {
        g.SmoothingMode = SmoothingMode.AntiAlias;
        g.InterpolationMode = InterpolationMode.HighQualityBilinear;
        g.PixelOffsetMode = PixelOffsetMode.HighQuality;
        g.CompositingQuality = CompositingQuality.HighQuality;
        g.TextRenderingHint = System.Drawing.Text.TextRenderingHint.AntiAliasGridFit;
    }

    /// <summary>PlayStation face-button glyphs drawn as vector shapes.</summary>
    public static void Glyph(Graphics g, int kind, RectangleF r, Color c, float w = 1.8f)
    {
        using var pen = new Pen(c, w) { LineJoin = LineJoin.Round, StartCap = LineCap.Round, EndCap = LineCap.Round };
        float cx = r.X + r.Width / 2f, cy = r.Y + r.Height / 2f, s = Math.Min(r.Width, r.Height) / 2f;
        switch (kind)
        {
            case 0: // triangle
                g.DrawPolygon(pen, new[] { new PointF(cx, cy - s), new PointF(cx + s, cy + s * 0.82f), new PointF(cx - s, cy + s * 0.82f) });
                break;
            case 1: // circle
                g.DrawEllipse(pen, cx - s, cy - s, s * 2, s * 2);
                break;
            case 2: // cross
                g.DrawLine(pen, cx - s * .9f, cy - s * .9f, cx + s * .9f, cy + s * .9f);
                g.DrawLine(pen, cx + s * .9f, cy - s * .9f, cx - s * .9f, cy + s * .9f);
                break;
            case 3: // square
                g.DrawRectangle(pen, cx - s * .85f, cy - s * .85f, s * 1.7f, s * 1.7f);
                break;
            case 4: // play
                using (var b = new SolidBrush(c))
                    g.FillPolygon(b, new[] { new PointF(cx - s * .65f, cy - s), new PointF(cx + s * .95f, cy), new PointF(cx - s * .65f, cy + s) });
                break;
            case 5: // copy (two sheets)
                g.DrawRectangle(pen, cx - s * .95f, cy - s * .95f, s * 1.2f, s * 1.3f);
                g.DrawRectangle(pen, cx - s * .25f, cy - s * .35f, s * 1.2f, s * 1.3f);
                break;
            default: // stop
                using (var b = new SolidBrush(c))
                using (var path = Round(new RectangleF(cx - s * .8f, cy - s * .8f, s * 1.6f, s * 1.6f), s * .25f))
                    g.FillPath(b, path);
                break;
        }
    }
}

internal static class UiNative
{
    [DllImport("dwmapi.dll")] static extern int DwmSetWindowAttribute(IntPtr hwnd, int attr, ref int val, int size);
    [DllImport("uxtheme.dll", CharSet = CharSet.Unicode)] static extern int SetWindowTheme(IntPtr hwnd, string? app, string? idList);

    /// <summary>Dark title bar (+ matching caption colour on Win11). Silently ignored where unsupported.</summary>
    public static void DarkTitleBar(IntPtr hwnd)
    {
        try
        {
            int on = 1;
            DwmSetWindowAttribute(hwnd, 20, ref on, 4);
            DwmSetWindowAttribute(hwnd, 19, ref on, 4);
            int cap = Theme.Void.R | (Theme.Void.G << 8) | (Theme.Void.B << 16);
            DwmSetWindowAttribute(hwnd, 35, ref cap, 4);
            int txt = Theme.TextPri.R | (Theme.TextPri.G << 8) | (Theme.TextPri.B << 16);
            DwmSetWindowAttribute(hwnd, 36, ref txt, 4);
        }
        catch { }
    }

    /// <summary>Dark native scrollbars / combo / spinner.</summary>
    public static void DarkControl(Control c, string theme = "DarkMode_Explorer")
    {
        void Apply() { try { SetWindowTheme(c.Handle, theme, null); } catch { } }
        if (c.IsHandleCreated) Apply(); else c.HandleCreated += (_, _) => Apply();
    }
}

/// <summary>Double-buffered transparent table used for layout only.</summary>
internal sealed class FlowGrid : TableLayoutPanel
{
    public FlowGrid()
    {
        DoubleBuffered = true;
        BackColor = Color.Transparent;
        SetStyle(ControlStyles.SupportsTransparentBackColor, true);
    }

    // Glass panels fully cover the cells and paint their own backdrop; skipping this avoids redundant repaints.
    protected override void OnPaintBackground(PaintEventArgs e) { }
}
