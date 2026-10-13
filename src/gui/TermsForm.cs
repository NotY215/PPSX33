using System.Text;
using System.Text.RegularExpressions;

namespace PS3Recomp.Gui;

/// <summary>
/// First-run gate: user must read TERMS.md and check "I have read..." before using the app.
/// Display text personalizes You/User with the Windows account name.
/// </summary>
public sealed class TermsForm : Form
{
    private readonly CheckBox _readBox = new();
    private readonly Button _accept = new();
    private readonly Button _leave = new();
    private readonly TextBox _body = new();
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
        Width = 720;
        Height = 640;
        MinimumSize = new Size(560, 480);
        StartPosition = FormStartPosition.CenterScreen;
        FormBorderStyle = FormBorderStyle.FixedDialog;
        MaximizeBox = false;
        MinimizeBox = false;
        ShowInTaskbar = true;
        BackColor = Color.FromArgb(6, 8, 18);
        ForeColor = Color.FromArgb(238, 242, 255);
        Font = new Font("Segoe UI", 9.25f);

        var title = new Label
        {
            Text = "Please read the Terms and Conditions before continuing",
            Dock = DockStyle.Top,
            Height = 36,
            TextAlign = ContentAlignment.MiddleLeft,
            Padding = new Padding(12, 0, 0, 0),
            ForeColor = Color.FromArgb(238, 242, 255),
            Font = new Font("Segoe UI", 11f, FontStyle.Bold)
        };

        _who.Text = "Signed in as Windows user: " + user;
        _who.Dock = DockStyle.Top;
        _who.Height = 28;
        _who.Padding = new Padding(12, 0, 0, 0);
        _who.ForeColor = Color.FromArgb(150, 162, 196);

        _body.Multiline = true;
        _body.ReadOnly = true;
        _body.ScrollBars = ScrollBars.Vertical;
        _body.Dock = DockStyle.Fill;
        _body.BackColor = Color.FromArgb(14, 20, 44);
        _body.ForeColor = Color.FromArgb(238, 242, 255);
        _body.BorderStyle = BorderStyle.FixedSingle;
        _body.Font = new Font("Consolas", 9.5f);
        _body.WordWrap = true;
        _body.Text = LoadPersonalizedTerms(user);

        var bottom = new Panel
        {
            Dock = DockStyle.Bottom,
            Height = 96,
            Padding = new Padding(12),
            BackColor = Color.FromArgb(10, 17, 37)
        };

        _readBox.Text = "I have read the Terms and Conditions properly";
        _readBox.AutoSize = true;
        _readBox.ForeColor = Color.FromArgb(238, 242, 255);
        _readBox.Location = new Point(12, 12);
        _readBox.CheckedChanged += (_, _) => _accept.Enabled = _readBox.Checked;

        _accept.Text = "Accept and continue";
        _accept.Enabled = false;
        _accept.Width = 160;
        _accept.Height = 34;
        _accept.Location = new Point(12, 48);
        _accept.BackColor = Color.FromArgb(38, 112, 210);
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
        _leave.Height = 34;
        _leave.Location = new Point(184, 48);
        _leave.BackColor = Color.FromArgb(210, 55, 78);
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

        Controls.Add(_body);
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

    static string LoadPersonalizedTerms(string user)
    {
        string raw = FindTermsText();
        return Personalize(raw, user);
    }

    static string FindTermsText()
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
        return
            "# PPSX33 — Terms and Conditions\n\n" +
            "TERMS.md was not found next to the application.\n" +
            "By continuing, " + WindowsUserName + " agrees to use only lawfully owned materials,\n" +
            "not to redistribute game data, and acknowledges PPSX33 ships no proprietary decrypt keys.\n" +
            "See the repository LEGAL.md and TERMS.md for the full text.\n";
    }

    public static string Personalize(string markdown, string user)
    {
        if (string.IsNullOrEmpty(markdown)) return markdown;
        string u = string.IsNullOrWhiteSpace(user) ? "User" : user.Trim();
        string s = markdown;
        s = Regex.Replace(s, @"\bUser\b", u);
        s = Regex.Replace(s, @"\byou\b", u);
        s = Regex.Replace(s, @"\bYou\b", u);
        s = Regex.Replace(s, @"\bYOUR\b", u.ToUpperInvariant());
        s = Regex.Replace(s, @"\byour\b", u + "'s");
        s = Regex.Replace(s, @"\bYour\b", u + "'s");
        return s;
    }
}
