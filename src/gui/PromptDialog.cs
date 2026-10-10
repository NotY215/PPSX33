using PS3Recomp.Gui.Ui;

namespace PS3Recomp.Gui;

/// <summary>Glass modal text prompt (project name, etc.).</summary>
internal sealed class PromptDialog : Form
{
    private readonly TextBox _box = new();
    public string Value => _box.Text.Trim();

    protected override void OnHandleCreated(EventArgs e)
    {
        base.OnHandleCreated(e);
        UiNative.DarkTitleBar(Handle);
    }

    public PromptDialog(string title, string label)
    {
        AutoScaleMode = AutoScaleMode.Dpi;
        AutoScaleDimensions = new SizeF(96f, 96f);
        Text = title;
        Width = 480;
        Height = 250;
        StartPosition = FormStartPosition.CenterParent;
        FormBorderStyle = FormBorderStyle.FixedDialog;
        MaximizeBox = false;
        MinimizeBox = false;
        ShowInTaskbar = false;
        BackColor = Theme.Void;
        ForeColor = Theme.TextPri;
        Font = Theme.Font(9.25f);
        DoubleBuffered = true;

        var stage = new LiquidBackdrop { Dock = DockStyle.Fill, Padding = new Padding(8) };
        var card = new GlassPanel { Dock = DockStyle.Fill, Radius = 20 };
        card.SetInset(20, 16, 20, 16);

        var lbl = new Label
        {
            Text = label,
            Dock = DockStyle.Top,
            Height = 30,
            ForeColor = Theme.TextSec,
            BackColor = Color.Transparent,
            Font = Theme.Semi(9.5f),
            TextAlign = ContentAlignment.BottomLeft
        };

        _box.BorderStyle = BorderStyle.None;
        _box.BackColor = Color.FromArgb(16, 22, 46);
        _box.ForeColor = Theme.TextPri;
        _box.Font = Theme.Font(10.5f);
        var field = new FieldFrame { Dock = DockStyle.Top, Height = 42, Fill = Color.FromArgb(16, 22, 46), Padding = new Padding(12, 0, 10, 0) };
        field.Controls.Add(_box);

        var ok = new GlassButton { Text = "OK", Kind = GlassKind.Primary, Glyph = 2, DialogResult = DialogResult.OK, Width = 150, Height = 40 };
        var cancel = new GlassButton { Text = "Cancel", Kind = GlassKind.Step, Glyph = 1, DialogResult = DialogResult.Cancel, Width = 150, Height = 40 };

        var buttons = new FlowLayoutPanel
        {
            Dock = DockStyle.Bottom,
            Height = 48,
            FlowDirection = FlowDirection.RightToLeft,
            BackColor = Color.Transparent
        };
        buttons.Controls.Add(ok);
        buttons.Controls.Add(cancel);

        card.Controls.Add(buttons);
        card.Controls.Add(field);
        card.Controls.Add(lbl);
        stage.Controls.Add(card);
        Controls.Add(stage);
        AcceptButton = ok;
        CancelButton = cancel;
        Shown += (_, _) => _box.Focus();
    }
}
