using System.Net;
using System.Text;
using System.Text.RegularExpressions;

namespace PS3Recomp.Gui;

/// <summary>Markdown → dark HTML for Terms dialog and Legal tab.</summary>
public static class MarkdownHtml
{
    public static string ToHtml(string md, string? bannerHtml = null)
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
    font-size: 14px; line-height: 1.65; }
  .wrap { padding: 20px 28px 36px; max-width: 900px; }
  h1 { font-size: 1.7em; border-bottom: 1px solid #30363d; padding-bottom: 0.3em;
    margin: 0.5em 0 0.55em; color: #f0f6fc; font-weight: 600; }
  h2 { font-size: 1.28em; border-bottom: 1px solid #21262d; padding-bottom: 0.22em;
    margin: 1.15em 0 0.45em; color: #f0f6fc; font-weight: 600; }
  h3 { font-size: 1.1em; margin: 1em 0 0.35em; color: #e6edf3; }
  p { margin: 0.6em 0; color: #c9d1d9; }
  ul, ol { margin: 0.45em 0 0.55em 1.35em; padding: 0; color: #c9d1d9; }
  li { margin: 0.28em 0; }
  code { background: #161b22; padding: 0.12em 0.38em; border-radius: 6px;
    font-family: ui-monospace, Consolas, "Courier New", monospace; font-size: 0.9em;
    color: #79c0ff; border: 1px solid #30363d; }
  a { color: #58a6ff; text-decoration: none; }
  a:hover { text-decoration: underline; }
  hr { border: none; border-top: 1px solid #30363d; margin: 1.25em 0; }
  strong { color: #f0f6fc; font-weight: 600; }
  .banner { background: linear-gradient(135deg, #161b22 0%, #1c2129 100%);
    border: 1px solid #30363d; border-radius: 10px; padding: 14px 18px;
    margin-bottom: 20px; color: #8b949e; box-shadow: 0 4px 24px rgba(0,0,0,.25); }
  .banner b { color: #e6edf3; }
  .card { background: #161b22; border: 1px solid #30363d; border-radius: 10px;
    padding: 14px 18px; margin: 12px 0; }
</style></head><body><div class="wrap">
""");
        if (!string.IsNullOrEmpty(bannerHtml))
            sb.Append("<div class=\"banner\">").Append(bannerHtml).Append("</div>\n");

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
            if (t.Length == 0) { CloseP(); CloseLists(); continue; }
            if (t.StartsWith("---") && t.Trim('-', ' ').Length == 0)
            { CloseP(); CloseLists(); sb.Append("<hr/>\n"); continue; }
            if (t.StartsWith("# "))
            { CloseP(); CloseLists(); sb.Append("<h1>").Append(Inline(t[2..])).Append("</h1>\n"); continue; }
            if (t.StartsWith("## "))
            { CloseP(); CloseLists(); sb.Append("<h2>").Append(Inline(t[3..])).Append("</h2>\n"); continue; }
            if (t.StartsWith("### "))
            { CloseP(); CloseLists(); sb.Append("<h3>").Append(Inline(t[4..])).Append("</h3>\n"); continue; }
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
            else sb.Append(' ');
            sb.Append(Inline(t));
        }
        CloseP(); CloseLists();
        sb.Append("</div></body></html>");
        return sb.ToString();
    }

    static string Inline(string s)
    {
        s = WebUtility.HtmlEncode(s);
        s = Regex.Replace(s, @"`([^`]+)`", "<code>$1</code>");
        s = Regex.Replace(s, @"\*\*([^*]+)\*\*", "<strong>$1</strong>");
        s = Regex.Replace(s, @"\[([^\]]+)\]\(([^)]+)\)", "<a href=\"$2\">$1</a>");
        return s;
    }
}
