namespace PS3Recomp.Gui;

internal sealed class PromptDialog : Form
{
    private readonly TextBox _box = new();
    public string Value => _box.Text.Trim();

    public PromptDialog(string title, string initial)
    {
        Text = title; Width = 420; Height = 160;
        StartPosition = FormStartPosition.CenterParent;
        FormBorderStyle = FormBorderStyle.FixedDialog;
        MaximizeBox = false; MinimizeBox = false; ShowInTaskbar = false;
        BackColor = Color.FromArgb(28, 28, 34);
        ForeColor = Color.FromArgb(230, 232, 240);
        Font = new Font("Segoe UI", 9.25f);
        var lbl = new Label { Text = "Name", ForeColor = Color.FromArgb(140, 145, 160), Location = new Point(16, 16), AutoSize = true };
        _box.Text = initial; _box.Location = new Point(16, 40); _box.Width = 370;
        _box.BackColor = Color.FromArgb(18, 18, 22); _box.ForeColor = Color.FromArgb(230, 232, 240); _box.BorderStyle = BorderStyle.FixedSingle;
        var ok = new Button { Text = "OK", DialogResult = DialogResult.OK, Location = new Point(220, 80), Width = 80, FlatStyle = FlatStyle.Flat, BackColor = Color.FromArgb(70, 130, 255), ForeColor = Color.White };
        ok.FlatAppearance.BorderSize = 0;
        var cancel = new Button { Text = "Cancel", DialogResult = DialogResult.Cancel, Location = new Point(306, 80), Width = 80, FlatStyle = FlatStyle.Flat, BackColor = Color.FromArgb(50, 50, 58), ForeColor = Color.White };
        cancel.FlatAppearance.BorderSize = 0;
        Controls.Add(lbl); Controls.Add(_box); Controls.Add(ok); Controls.Add(cancel);
        AcceptButton = ok; CancelButton = cancel;
    }
}
