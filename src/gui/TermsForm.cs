using System.Text;
using System.Text.RegularExpressions;
using System.Net;

namespace PS3Recomp.Gui;

/// <summary>
/// First-run gate: TERMS.md rendered as readable HTML (GitHub/VS Code style).
/// Windows username shown in banner; body keeps proper spacing and structure.
/// </summary>
public sealed class TermsForm : Form
{
    private readonly CheckBox _readBox = new();
    private readonly Button _accept = new();
    private readonly Button _leave = new();
    private readonly WebBrowser _browser = new();
    private readonly Label _who = new();

    public bool Accepted { get; private set; }

    public static string WindowsUserName
    {
        get
        {
            try
            {
                string u = Environment.UserName;
                if (!string.IsNullOrWhiteSpace(u)) return u.Trim();
            }
            catch { }
            return "User";
        }
    }

    public TermsForm()
    {
        string user = WindowsUserName;
        Text = "PPSX33 — Terms and Conditions";
        Width = 780;
        Height = 680;
        MinimumSize = new Size(640, 520);
        StartPosition = FormStartPosition.CenterScreen;
        FormBorderStyle = FormBorderStyle.FixedDialog;
        MaximizeBox = false;
        MinimizeBox = false;
        ShowInTaskbar = true;
        BackColor = Color.FromArgb(13, 17, 23);
        ForeColor = Color.FromArgb(230, 237, 243);
        Font = new Font("Segoe UI", 9.5f);

        var title = new Label
        {
            Text = "Please read the Terms and Conditions before continuing",
            Dock = DockStyle.Top,
            Height = 40,
            TextAlign = ContentAlignment.MiddleLeft,
            Padding = new Padding(16, 0, 0, 0),
            ForeColor = Color.FromArgb(230, 237, 243),
            Font = new Font("Segoe UI", 12f, FontStyle.Bold),
            BackColor = Color.FromArgb(22, 27, 34)
        };

        _who.Text = "Signed in as Windows user: " + user +
                    "  ·  Terms apply to " + user + " personally";
        _who.Dock = DockStyle.Top;
        _who.Height = 28;
        _who.Padding = new Padding(16, 0, 0, 0);
        _who.ForeColor = Color.FromArgb(139, 148, 158);
        _who.BackColor = Color.FromArgb(22, 27, 34);

        _browser.Dock = DockStyle.Fill;
        _browser.IsWebBrowserContextMenuEnabled = false;
        _browser.AllowWebBrowserDrop = false;
        _browser.ScriptErrorsSuppressed = true;
        _browser.ScrollBarsEnabled = true;
        string md = LoadTermsRaw();
        string html = MarkdownToHtml(md, user);
        _browser.DocumentText = html;

        var bottom = new Panel
        {
            Dock = DockStyle.Bottom,
            Height = 100,
            Padding = new Padding(16),
            BackColor = Color.FromArgb(22, 27, 34)
        };

        _readBox.Text = "I have read the Terms and Conditions properly";
        _readBox.AutoSize = true;
        _readBox.ForeColor = Color.FromArgb(230, 237, 243);
        _readBox.Location = new Point(16, 14);
        _readBox.CheckedChanged += (_, _) => _accept.Enabled = _readBox.Checked;

        _accept.Text = "Accept and continue";
        _accept.Enabled = false;
        _accept.Width = 170;
        _accept.Height = 36;
        _accept.Location = new Point(16, 52);
        _accept.BackColor = Color.FromArgb(35, 134, 54);
        _accept.ForeColor = Color.White;
        _accept.FlatStyle = FlatStyle.Flat;
        _accept.Click += (_, _) =>
        {
            Accepted = true;
            DialogResult = DialogResult.OK;
            Close();
        };

        _leave.Text = "Leave the app";
        _leave.Width = 140;
        _leave.Height = 36;
        _leave.Location = new Point(200, 52);
        _leave.BackColor = Color.FromArgb(218, 54, 51);
        _leave.ForeColor = Color.White;
        _leave.FlatStyle = FlatStyle.Flat;
        _leave.Click += (_, _) =>
        {
            Accepted = false;
            DialogResult = DialogResult.Cancel;
            Close();
        };

        bottom.Controls.Add(_readBox);
        bottom.Controls.Add(_accept);
        bottom.Controls.Add(_leave);

        Controls.Add(_browser);
        Controls.Add(bottom);
        Controls.Add(_who);
        Controls.Add(title);

        FormClosing += (_, e) =>
        {
            if (!Accepted && DialogResult != DialogResult.Cancel)
            {
                Accepted = false;
                DialogResult = DialogResult.Cancel;
            }
        };
    }

    static string LoadTermsRaw()
    {
        string[] candidates =
        {
            Path.Combine(AppContext.BaseDirectory, "TERMS.md"),
            Path.Combine(AppContext.BaseDirectory, "..", "TERMS.md"),
            Path.Combine(AppContext.BaseDirectory, "..", "..", "TERMS.md"),
            Path.Combine(AppContext.BaseDirectory, "..", "..", "..", "TERMS.md"),
        };
        foreach (var p in candidates)
        {
            try
            {
                string full = Path.GetFullPath(p);
                if (File.Exists(full))
                    return File.ReadAllText(full, Encoding.UTF8);
            }
            catch { }
        }
        return "# PPSX33 — Terms and Conditions\n\n"
             + "TERMS.md was not found next to the application.\n\n"
             + "By continuing you agree to use only lawfully owned materials, "
             + "not to redistribute game data, and acknowledge that PPSX33 ships "
             + "no proprietary decrypt keys. See repository LEGAL.md and TERMS.md.\n";
    }

    /// <summary>Minimal Markdown → HTML (headings, bold, italic, code, lists, links, paragraphs).</summary>
    public static string MarkdownToHtml(string md, string user)
    {
        if (string.IsNullOrEmpty(md)) md = "";
        md = md.Replace("\r\n", "\n").Replace("\r", "\n");

        var sb = new StringBuilder();
        sb.Append("""
<!DOCTYPE html>
<html><head><meta charset="utf-8"/>
<style>
  html, body { margin: 0; padding: 0; background: #0d1117; color: #e6edf3;
    font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Helvetica, Arial, sans-serif;
    font-size: 14px; line-height: 1.6; }
  .wrap { padding: 20px 28px 32px; max-width: 860px; }
  h1 { font-size: 1.75em; border-bottom: 1px solid #30363d; padding-bottom: 0.3em; margin: 0.6em 0 0.5em; }
  h2 { font-size: 1.35em; border-bottom: 1px solid #21262d; padding-bottom: 0.25em; margin: 1.2em 0 0.5em; }
  h3 { font-size: 1.15em; margin: 1em 0 0.4em; }
  p { margin: 0.65em 0; }
  ul, ol { margin: 0.5em 0 0.5em 1.4em; padding: 0; }
  li { margin: 0.25em 0; }
  code { background: #161b22; padding: 0.15em 0.4em; border-radius: 4px;
    font-family: ui-monospace, Consolas, "Courier New", monospace; font-size: 0.92em; }
  a { color: #58a6ff; text-decoration: none; }
  a:hover { text-decoration: underline; }
  hr { border: none; border-top: 1px solid #30363d; margin: 1.2em 0; }
  strong { color: #f0f6fc; }
  .banner { background: #161b22; border: 1px solid #30363d; border-radius: 8px;
    padding: 12px 16px; margin-bottom: 18px; color: #8b949e; }
  .banner b { color: #e6edf3; }
</style></head><body><div class="wrap">
""");
        sb.Append("<div class=\"banner\">These terms apply to Windows user <b>")
          .Append(WebUtility.HtmlEncode(user))
          .Append("</b>. Read carefully before accepting.</div>\n");

        var lines = md.Split('\n');
        bool inUl = false, inOl = false, inP = false;

        void CloseLists()
        {
            if (inUl) { sb.Append("</ul>\n"); inUl = false; }
            if (inOl) { sb.Append("</ol>\n"); inOl = false; }
        }
        void CloseP()
        {
            if (inP) { sb.Append("</p>\n"); inP = false; }
        }

        foreach (var raw in lines)
        {
            string trimmed = raw.TrimEnd();
            string t = trimmed.TrimStart();

            if (t.Length == 0)
            {
                CloseP();
                CloseLists();
                continue;
            }

            if (t.StartsWith("---") && t.Trim('-').Length == 0)
            {
                CloseP(); CloseLists();
                sb.Append("<hr/>\n");
                continue;
            }

            if (t.StartsWith("# "))
            {
                CloseP(); CloseLists();
                sb.Append("<h1>").Append(Inline(t[2..])).Append("</h1>\n");
                continue;
            }
            if (t.StartsWith("## "))
            {
                CloseP(); CloseLists();
                sb.Append("<h2>").Append(Inline(t[3..])).Append("</h2>\n");
                continue;
            }
            if (t.StartsWith("### "))
            {
                CloseP(); CloseLists();
                sb.Append("<h3>").Append(Inline(t[4..])).Append("</h3>\n");
                continue;
            }

            if (t.StartsWith("- ") || t.StartsWith("* "))
            {
                CloseP();
                if (inOl) { sb.Append("</ol>\n"); inOl = false; }
                if (!inUl) { sb.Append("<ul>\n"); inUl = true; }
                sb.Append("<li>").Append(Inline(t[2..])).Append("</li>\n");
                continue;
            }

            var om = Regex.Match(t, @"^(\d+)\.\s+(.*)$");
            if (om.Success)
            {
                CloseP();
                if (inUl) { sb.Append("</ul>\n"); inUl = false; }
                if (!inOl) { sb.Append("<ol>\n"); inOl = true; }
                sb.Append("<li>").Append(Inline(om.Groups[2].Value)).Append("</li>\n");
                continue;
            }

            CloseLists();
            if (!inP) { sb.Append("<p>"); inP = true; }
            else sb.Append(" ");
            sb.Append(Inline(t));
            if (trimmed.EndsWith("  "))
                sb.Append("<br/>");
        }
        CloseP();
        CloseLists();
        sb.Append("</div></body></html>");
        return sb.ToString();
    }

    static string Inline(string s)
    {
        s = WebUtility.HtmlEncode(s);
        s = Regex.Replace(s, @"\[([^\]]+)\]\(([^)]+)\)",
            m => "<a href=\"" + m.Groups[2].Value + "\">" + m.Groups[1].Value + "</a>");
        s = Regex.Replace(s, @"\*\*(.+?)\*\*", "<strong>$1</strong>");
        s = Regex.Replace(s, @"(?<!\*)\*(?!\*)(.+?)(?<!\*)\*(?!\*)", "<em>$1</em>");
        s = Regex.Replace(s, @"`([^`]+)`", "<code>$1</code>");
        return s;
    }
}
