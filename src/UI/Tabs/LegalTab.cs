using System.Drawing;
using System.Windows.Forms;

namespace PS3Recomp.Gui;

/// <summary>Legal tab body — RichTextBox (reliable; avoids IE WebBrowser blank DocumentText).</summary>
public sealed class LegalTab : UserControl
{
    private readonly RichTextBox _box = new();

    public LegalTab()
    {
        Dock = DockStyle.Fill;
        BackColor = Color.FromArgb(13, 17, 23);
        _box.Dock = DockStyle.Fill;
        _box.ReadOnly = true;
        _box.BorderStyle = BorderStyle.None;
        _box.BackColor = Color.FromArgb(13, 17, 23);
        _box.ForeColor = Color.FromArgb(230, 237, 243);
        _box.Font = new Font("Segoe UI", 10f);
        _box.DetectUrls = true;
        _box.ScrollBars = RichTextBoxScrollBars.Vertical;
        _box.Text = FormatLegal(EmbeddedLegal.Markdown);
        _box.LinkClicked += (_, e) =>
        {
            try
            {
                System.Diagnostics.Process.Start(new System.Diagnostics.ProcessStartInfo(e.LinkText)
                {
                    UseShellExecute = true
                });
            }
            catch { }
        };
        Controls.Add(_box);
    }

    static string FormatLegal(string md)
    {
        if (string.IsNullOrEmpty(md)) return "";
        var lines = md.Replace("\r\n", "\n").Split('\n');
        var sb = new System.Text.StringBuilder();
        foreach (var raw in lines)
        {
            string t = raw.TrimEnd();
            if (t.StartsWith("# "))
            {
                sb.AppendLine();
                sb.AppendLine(t[2..].Trim());
                sb.AppendLine(new string('═', Math.Min(48, t.Length)));
                continue;
            }
            if (t.StartsWith("## "))
            {
                sb.AppendLine();
                sb.AppendLine(t[3..].Trim());
                sb.AppendLine(new string('─', Math.Min(40, t.Length)));
                continue;
            }
            if (t.StartsWith("### "))
            {
                sb.AppendLine();
                sb.AppendLine(t[4..].Trim());
                continue;
            }
            if (t.StartsWith("- ") || t.StartsWith("* "))
            {
                sb.AppendLine("  • " + t[2..].Trim());
                continue;
            }
            if (t.StartsWith("---"))
            {
                sb.AppendLine();
                continue;
            }
            // strip simple markdown emphasis for plain view
            string p = t.Replace("**", "").Replace("`", "");
            if (p.Length == 0) sb.AppendLine();
            else sb.AppendLine(p);
        }
        return sb.ToString().TrimStart();
    }
}
