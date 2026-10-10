namespace PS3Recomp.Gui;

/// <summary>Dark modal text prompt (project name, etc.).</summary>
internal sealed class PromptDialog : Form
{
    private readonly TextBox _box = new();
    public string Value => _box.Text.Trim();

    public PromptDialog(string title, string label)
    {
        Text = title;
        Width = 440;
        Height = 170;
        StartPosition = FormStartPosition.CenterParent;
        FormBorderStyle = FormBorderStyle.FixedDialog;
        MaximizeBox = false;
        MinimizeBox = false;
        BackColor = Color.FromArgb(28, 28, 32);
        ForeColor = Color.FromArgb(230, 230, 235);
        Font = new Font("Segoe UI", 9.25f);

        var lbl = new Label
        {
            Text = label,
            Dock = DockStyle.Top,
            Height = 36,
            Padding = new Padding(12, 12, 12, 0),
            ForeColor = Color.FromArgb(150, 152, 160)
        };
        _box.Dock = DockStyle.Top;
        _box.Height = 28;
        _box.Margin = new Padding(12);
        _box.BackColor = Color.FromArgb(36, 36, 42);
        _box.ForeColor = Color.FromArgb(230, 230, 235);
        _box.BorderStyle = BorderStyle.FixedSingle;

        var ok = new Button
        {
            Text = "OK",
            DialogResult = DialogResult.OK,
            Dock = DockStyle.Bottom,
            Height = 36,
            FlatStyle = FlatStyle.Flat,
            BackColor = Color.FromArgb(45, 125, 255),
            ForeColor = Color.White
        };
        ok.FlatAppearance.BorderSize = 0;

        var pad = new Panel { Dock = DockStyle.Fill, Padding = new Padding(12, 8, 12, 8) };
        pad.Controls.Add(_box);

        Controls.Add(ok);
        Controls.Add(pad);
        Controls.Add(lbl);
        AcceptButton = ok;
    }
}
