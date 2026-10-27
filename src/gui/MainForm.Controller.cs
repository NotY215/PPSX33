using System;
using System.Collections.Generic;
using System.Drawing;
using System.IO;
using System.Runtime.InteropServices;
using System.Text.Json;
using System.Windows.Forms;

namespace PS3Recomp.Gui;

public partial class MainForm
{
    private Panel? _controllerPanel;
    private TabPage? _controllerTab;
    private readonly List<TextBox> _mapBoxes = new();
    private readonly List<Action<string>> _mapSetters = new();
    private readonly List<Func<string>> _mapGetters = new();

    [DllImport("user32.dll")]
    private static extern short GetAsyncKeyState(int vKey);

    private const int VK_LSHIFT = 0xA0;
    private const int VK_RSHIFT = 0xA1;
    private const int VK_LCONTROL = 0xA2;
    private const int VK_RCONTROL = 0xA3;
    private const int VK_LMENU = 0xA4;
    private const int VK_RMENU = 0xA5;

    void BuildControllerTab()
    {
        _mapBoxes.Clear();
        _mapSetters.Clear();
        _mapGetters.Clear();

        _controllerPanel = new Panel
        {
            Dock = DockStyle.Fill,
            AutoScroll = true,
            BackColor = Color.FromArgb(7, 13, 24),
            Padding = new Padding(12)
        };

        var title = new Label
        {
            Text = "Keyboard → DualShock mapping",
            ForeColor = Color.FromArgb(51, 190, 220),
            Font = new Font("Segoe UI", 11f, FontStyle.Bold),
            AutoSize = true,
            Location = new Point(12, 10)
        };
        _controllerPanel.Controls.Add(title);

        var hint = new Label
        {
            Text = "Click a field and press a key. Left/Right Shift, Ctrl, Alt are recorded separately.",
            ForeColor = Color.FromArgb(144, 166, 190),
            AutoSize = true,
            Location = new Point(12, 36)
        };
        _controllerPanel.Controls.Add(hint);

        var grid = new TableLayoutPanel
        {
            Location = new Point(12, 64),
            ColumnCount = 2,
            AutoSize = true,
            AutoSizeMode = AutoSizeMode.GrowAndShrink
        };
        grid.ColumnStyles.Add(new ColumnStyle(SizeType.Absolute, 180));
        grid.ColumnStyles.Add(new ColumnStyle(SizeType.Absolute, 180));

        var km = _settings.KeyMap ?? KeyMap.Defaults();
        _settings.KeyMap = km;

        void AddRow(string label, Func<string> get, Action<string> set)
        {
            int row = grid.RowCount++;
            grid.RowStyles.Add(new RowStyle(SizeType.Absolute, 32));
            var lab = new Label
            {
                Text = label,
                ForeColor = Color.FromArgb(200, 210, 220),
                Dock = DockStyle.Fill,
                TextAlign = ContentAlignment.MiddleLeft
            };
            var tb = new TextBox
            {
                Text = get() ?? "",
                Dock = DockStyle.Fill,
                BackColor = Color.FromArgb(16, 27, 43),
                ForeColor = Color.White,
                BorderStyle = BorderStyle.FixedSingle,
                ReadOnly = true
            };
            tb.KeyDown += (_, e) =>
            {
                e.SuppressKeyPress = true;
                e.Handled = true;
                string name = FormatWinKey(e);
                if (name == "Escape") return;
                set(name);
                tb.Text = name;
            };
            grid.Controls.Add(lab, 0, row);
            grid.Controls.Add(tb, 1, row);
            _mapBoxes.Add(tb);
            _mapGetters.Add(get);
            _mapSetters.Add(set);
        }

        AddRow("Left stick Left", () => km.LeftStickLeft, v => km.LeftStickLeft = v);
        AddRow("Left stick Down", () => km.LeftStickDown, v => km.LeftStickDown = v);
        AddRow("Left stick Right", () => km.LeftStickRight, v => km.LeftStickRight = v);
        AddRow("Left stick Up", () => km.LeftStickUp, v => km.LeftStickUp = v);
        AddRow("Right stick Left", () => km.RightStickLeft, v => km.RightStickLeft = v);
        AddRow("Right stick Down", () => km.RightStickDown, v => km.RightStickDown = v);
        AddRow("Right stick Right", () => km.RightStickRight, v => km.RightStickRight = v);
        AddRow("Right stick Up", () => km.RightStickUp, v => km.RightStickUp = v);
        AddRow("Start", () => km.Start, v => km.Start = v);
        AddRow("Select", () => km.Select, v => km.Select = v);
        AddRow("PS button", () => km.PsButton, v => km.PsButton = v);
        AddRow("Square", () => km.Square, v => km.Square = v);
        AddRow("Cross", () => km.Cross, v => km.Cross = v);
        AddRow("Circle", () => km.Circle, v => km.Circle = v);
        AddRow("Triangle", () => km.Triangle, v => km.Triangle = v);
        AddRow("D-pad Left", () => km.DpadLeft, v => km.DpadLeft = v);
        AddRow("D-pad Down", () => km.DpadDown, v => km.DpadDown = v);
        AddRow("D-pad Right", () => km.DpadRight, v => km.DpadRight = v);
        AddRow("D-pad Up", () => km.DpadUp, v => km.DpadUp = v);
        AddRow("R1", () => km.R1, v => km.R1 = v);
        AddRow("R2", () => km.R2, v => km.R2 = v);
        AddRow("R3", () => km.R3, v => km.R3 = v);
        AddRow("L1", () => km.L1, v => km.L1 = v);
        AddRow("L2", () => km.L2, v => km.L2 = v);
        AddRow("L3", () => km.L3, v => km.L3 = v);

        _controllerPanel.Controls.Add(grid);

        int btnY = 64 + 25 * 32 + 20;
        var btnSave = new Button
        {
            Text = "Save mapping",
            Location = new Point(12, btnY),
            Size = new Size(140, 32),
            BackColor = Color.FromArgb(17, 65, 83),
            ForeColor = Color.White,
            FlatStyle = FlatStyle.Flat
        };
        btnSave.Click += (_, _) =>
        {
            _settings.KeyMap = km;
            SaveSettings();
            WritePadMapForRuntime();
            _controllerMapped = true;
            Append("Controller mapping saved.");
            UpdateButtons();
        };

        var btnDefault = new Button
        {
            Text = "Defaults",
            Location = new Point(164, btnY),
            Size = new Size(100, 32),
            BackColor = Color.FromArgb(16, 27, 43),
            ForeColor = Color.White,
            FlatStyle = FlatStyle.Flat
        };
        btnDefault.Click += (_, _) =>
        {
            var d = KeyMap.Defaults();
            km.LeftStickLeft = d.LeftStickLeft;
            km.LeftStickDown = d.LeftStickDown;
            km.LeftStickRight = d.LeftStickRight;
            km.LeftStickUp = d.LeftStickUp;
            km.RightStickLeft = d.RightStickLeft;
            km.RightStickDown = d.RightStickDown;
            km.RightStickRight = d.RightStickRight;
            km.RightStickUp = d.RightStickUp;
            km.Start = d.Start;
            km.Select = d.Select;
            km.PsButton = d.PsButton;
            km.Square = d.Square;
            km.Cross = d.Cross;
            km.Circle = d.Circle;
            km.Triangle = d.Triangle;
            km.DpadLeft = d.DpadLeft;
            km.DpadDown = d.DpadDown;
            km.DpadRight = d.DpadRight;
            km.DpadUp = d.DpadUp;
            km.R1 = d.R1;
            km.R2 = d.R2;
            km.R3 = d.R3;
            km.L1 = d.L1;
            km.L2 = d.L2;
            km.L3 = d.L3;
            for (int i = 0; i < _mapBoxes.Count && i < _mapGetters.Count; ++i)
                _mapBoxes[i].Text = _mapGetters[i]() ?? "";
            Append("Defaults restored — click Save mapping to write pad_map.json.");
        };

        _controllerPanel.Controls.Add(btnSave);
        _controllerPanel.Controls.Add(btnDefault);

        _controllerTab = new TabPage("Controller")
        {
            BackColor = Color.FromArgb(7, 13, 24),
            Padding = new Padding(0)
        };
        _controllerTab.Controls.Add(_controllerPanel);
        _tabs.TabPages.Add(_controllerTab);
    }

    /// <summary>Map WinForms key to pad_map name with L/R Shift/Ctrl/Alt separation.</summary>
    static string FormatWinKey(KeyEventArgs e)
    {
        Keys k = e.KeyCode;

        // Generic modifier keys — resolve side via Win32
        if (k == Keys.ShiftKey || k == Keys.LShiftKey || k == Keys.RShiftKey)
        {
            if ((GetAsyncKeyState(VK_RSHIFT) & 0x8000) != 0) return "RShiftKey";
            if ((GetAsyncKeyState(VK_LSHIFT) & 0x8000) != 0) return "LShiftKey";
            return e.KeyCode == Keys.RShiftKey ? "RShiftKey" : "LShiftKey";
        }
        if (k == Keys.ControlKey || k == Keys.LControlKey || k == Keys.RControlKey)
        {
            if ((GetAsyncKeyState(VK_RCONTROL) & 0x8000) != 0) return "RControlKey";
            if ((GetAsyncKeyState(VK_LCONTROL) & 0x8000) != 0) return "LControlKey";
            return e.KeyCode == Keys.RControlKey ? "RControlKey" : "LControlKey";
        }
        if (k == Keys.Menu || k == Keys.LMenu || k == Keys.RMenu)
        {
            if ((GetAsyncKeyState(VK_RMENU) & 0x8000) != 0) return "RMenu";
            if ((GetAsyncKeyState(VK_LMENU) & 0x8000) != 0) return "LMenu";
            return e.KeyCode == Keys.RMenu ? "RMenu" : "LMenu";
        }

        return k switch
        {
            Keys.Left => "Left",
            Keys.Right => "Right",
            Keys.Up => "Up",
            Keys.Down => "Down",
            Keys.Space => "Space",
            Keys.Back => "Backspace",
            Keys.Enter => "Enter",
            Keys.Escape => "Escape",
            Keys.Tab => "Tab",
            Keys.Delete => "Delete",
            Keys.Insert => "Insert",
            Keys.Home => "Home",
            Keys.End => "End",
            Keys.PageUp => "PageUp",
            Keys.PageDown => "PageDown",
            Keys.NumPad0 => "NumPad0",
            Keys.NumPad1 => "NumPad1",
            Keys.NumPad2 => "NumPad2",
            Keys.NumPad3 => "NumPad3",
            Keys.NumPad4 => "NumPad4",
            Keys.NumPad5 => "NumPad5",
            Keys.NumPad6 => "NumPad6",
            Keys.NumPad7 => "NumPad7",
            Keys.NumPad8 => "NumPad8",
            Keys.NumPad9 => "NumPad9",
            _ => k.ToString()
        };
    }

    void WritePadMapForRuntime()
    {
        try
        {
            if (string.IsNullOrEmpty(_projectDir))
            {
                Append("Key map saved to settings; load a project to also write output/pad_map.json.");
                return;
            }
            string outDir = Path.Combine(_projectDir, "output");
            Directory.CreateDirectory(outDir);
            var km = _settings.KeyMap ?? KeyMap.Defaults();
            string path = Path.Combine(outDir, "pad_map.json");
            File.WriteAllText(path, JsonSerializer.Serialize(km, new JsonSerializerOptions { WriteIndented = true }));
            Append("Wrote pad_map.json → " + path);
        }
        catch (Exception ex)
        {
            Append("pad_map write failed: " + ex.Message);
        }
    }

    void HideControllerAfterBuild()
    {
        if (_controllerHiddenAfterBuild) return;
        _controllerHiddenAfterBuild = true;
        _controllerMapped = true;
        try
        {
            if (_controllerTab != null && _tabs.TabPages.Contains(_controllerTab))
            {
                if (_tabs.SelectedTab == _controllerTab) _tabs.SelectedIndex = 0;
                _tabs.TabPages.Remove(_controllerTab);
            }
            Append("Controller tab closed — mapping locked for this build.");
        }
        catch { }
    }

    void DisposeNativeController() { }
}
