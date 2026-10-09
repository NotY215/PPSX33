namespace PS3Recomp.Gui;

/// <summary>Tiny modal "enter text" dialog (used to ask for the game name).</summary>
internal sealed class PromptDialog : Form
{
    private readonly TextBox _box = new() { Dock = DockStyle.Top };
    public string Value => _box.Text.Trim();

    public PromptDialog(string title, string label)
    {
        Text = title; Width = 420; Height = 160; StartPosition = FormStartPosition.CenterParent;
        FormBorderStyle = FormBorderStyle.FixedDialog; MaximizeBox = false; MinimizeBox = false;
        var lbl = new Label { Text = label, Dock = DockStyle.Top, Height = 30 };
        var ok = new Button { Text = "OK", DialogResult = DialogResult.OK, Dock = DockStyle.Bottom, Height = 32 };
        Controls.Add(ok); Controls.Add(_box); Controls.Add(lbl);
        AcceptButton = ok;
    }
}
