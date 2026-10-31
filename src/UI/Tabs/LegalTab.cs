using System.Drawing;
using System.Windows.Forms;

namespace PS3Recomp.Gui;

/// <summary>Legal tab — styled HTML via WebBrowser + MarkdownHtml (original design).</summary>
public sealed class LegalTab : UserControl
{
    private readonly WebBrowser _browser = new();
    private bool _loaded;

    public LegalTab()
    {
        Dock = DockStyle.Fill;
        BackColor = Color.FromArgb(13, 17, 23);
        _browser.Dock = DockStyle.Fill;
        _browser.ScriptErrorsSuppressed = true;
        _browser.IsWebBrowserContextMenuEnabled = false;
        _browser.AllowWebBrowserDrop = false;
        _browser.WebBrowserShortcutsEnabled = false;
        _browser.Navigating += (_, e) =>
        {
            string url = e.Url?.AbsoluteUri ?? "";
            if (string.IsNullOrEmpty(url) || url.StartsWith("about:", StringComparison.OrdinalIgnoreCase))
                return;
            e.Cancel = true;
            try
            {
                string target = e.Url!.ToString();
                if (!string.IsNullOrWhiteSpace(target))
                    System.Diagnostics.Process.Start(new System.Diagnostics.ProcessStartInfo(target) { UseShellExecute = true });
            }
            catch { }
        };
        Controls.Add(_browser);
        HandleCreated += (_, _) => LoadHtml();
        VisibleChanged += (_, _) => { if (Visible) LoadHtml(); };
    }

    void LoadHtml()
    {
        if (_loaded || !IsHandleCreated) return;
        try
        {
            string html = MarkdownHtml.ToHtml(
                EmbeddedLegal.Markdown,
                "Legal notice for Windows user <b>" +
                System.Net.WebUtility.HtmlEncode(Environment.UserName) +
                "</b>. Read before Load / Build / Run.");
            _browser.DocumentText = html;
            _loaded = true;
        }
        catch
        {
            _browser.DocumentText = "<html><body style='background:#0d1117;color:#c9d1d9;font-family:Segoe UI'>"
                + "<pre>" + System.Net.WebUtility.HtmlEncode(EmbeddedLegal.Markdown) + "</pre></body></html>";
            _loaded = true;
        }
    }
}
