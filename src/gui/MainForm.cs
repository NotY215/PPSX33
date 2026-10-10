namespace PS3Recomp.Gui;

/// <summary>
/// PPSX33 main window. Dark professional layout inspired by Premiere Pro + RPCS3.
/// Workflow: Decrypt EBOOT -> Decompile -> Build -> Copy.
/// </summary>
public sealed class MainForm : Form
{
    // Palette (dark studio)
    static readonly Color Bg = Color.FromArgb(18, 18, 20);
    static readonly Color Panel = Color.FromArgb(28, 28, 32);
    static readonly Color Panel2 = Color.FromArgb(36, 36, 42);
    static readonly Color Border = Color.FromArgb(48, 48, 56);
    static readonly Color TextPri = Color.FromArgb(230, 230, 235);
    static readonly Color TextSec = Color.FromArgb(150, 152, 160);
    static readonly Color Accent = Color.FromArgb(45, 125, 255);
    static readonly Color AccentHover = Color.FromArgb(70, 145, 255);
    static readonly Color Success = Color.FromArgb(60, 180, 120);
    static readonly Color Danger = Color.FromArgb(220, 80, 80);

    private readonly Settings _settings = Settings.Load();
    private readonly TextBox _log = new();
    private readonly TextBox _report = new();
    private readonly Label _project = new();
    private readonly Label _status = new();
    private readonly ProgressBar _progress = new();
    private readonly Button _btnDecrypt = new();
    private readonly Button _btnLoad = new();
    private readonly Button _btnLift = new();
    private readonly Button _btnBuild = new();
    private readonly Button _btnCopy = new();
    private readonly Button _btnCancel = new();
    private readonly ComboBox _gfx = new();
    private readonly NumericUpDown _threads = new();
    private string? _projectDir;
    private CancellationTokenSource? _cts;
    private bool _canBuild;
    private bool _canCopy;

    private static string RuntimeDir => Path.Combine(AppContext.BaseDirectory, "runtime");
    private static string CompilersDir => Path.Combine(AppContext.BaseDirectory, "Compilers-files");

    public MainForm()
    {
        Text = "PPSX33  ·  PlayStation 3 Recompiler";
        Width = 1100;
        Height = 720;
        MinimumSize = new Size(900, 560);
        StartPosition = FormStartPosition.CenterScreen;
        BackColor = Bg;
        ForeColor = TextPri;
        Font = new Font("Segoe UI", 9.25f);

        // ---- Header ----
        var header = new Panel { Dock = DockStyle.Top, Height = 52, BackColor = Panel, Padding = new Padding(16, 0, 16, 0) };
        var title = new Label
        {
            Text = "PPSX33",
            Font = new Font("Segoe UI Semibold", 16f),
            ForeColor = TextPri,
            AutoSize = true,
            Location = new Point(16, 12)
        };
        var subtitle = new Label
        {
            Text = "Static recompiler  ·  PPU / SPU / RSX",
            ForeColor = TextSec,
            AutoSize = true,
            Location = new Point(110, 18)
        };
        header.Controls.Add(title);
        header.Controls.Add(subtitle);

        // ---- Left rail (workflow) ----
        var rail = new Panel { Dock = DockStyle.Left, Width = 220, BackColor = Panel, Padding = new Padding(12) };
        var railTitle = new Label
        {
            Text = "WORKFLOW",
            ForeColor = TextSec,
            Font = new Font("Segoe UI Semibold", 8f),
            Dock = DockStyle.Top,
            Height = 28,
            TextAlign = ContentAlignment.MiddleLeft
        };

        StylePrimaryButton(_btnDecrypt, "Decrypt EBOOT");
        StylePrimaryButton(_btnLoad, "Load ELF");
        StyleStepButton(_btnLift, "Decompile");
        StyleStepButton(_btnBuild, "Build");
        StyleStepButton(_btnCopy, "Copy to game folder");
        StyleDangerButton(_btnCancel, "Cancel");
        _btnLift.Enabled = false;
        _btnBuild.Enabled = false;
        _btnCopy.Enabled = false;
        _btnCancel.Enabled = false;

        var btnStack = new FlowLayoutPanel
        {
            Dock = DockStyle.Top,
            FlowDirection = FlowDirection.TopDown,
            WrapContents = false,
            AutoSize = true,
            Padding = new Padding(0, 4, 0, 0)
        };
        foreach (var b in new[] { _btnDecrypt, _btnLoad, _btnLift, _btnBuild, _btnCopy, _btnCancel })
        {
            b.Width = 190;
            b.Margin = new Padding(0, 0, 0, 8);
            btnStack.Controls.Add(b);
        }

        var optsTitle = new Label
        {
            Text = "OPTIONS",
            ForeColor = TextSec,
            Font = new Font("Segoe UI Semibold", 8f),
            Dock = DockStyle.Top,
            Height = 28,
            Margin = new Padding(0, 16, 0, 0),
            TextAlign = ContentAlignment.MiddleLeft
        };
        var optsPanel = new Panel { Dock = DockStyle.Top, Height = 100, BackColor = Panel };

        var gfxLbl = new Label { Text = "Graphics backend", ForeColor = TextSec, Location = new Point(0, 4), AutoSize = true };
        _gfx.DropDownStyle = ComboBoxStyle.DropDownList;
        _gfx.Width = 190;
        _gfx.Location = new Point(0, 24);
        _gfx.FlatStyle = FlatStyle.Flat;
        _gfx.BackColor = Panel2;
        _gfx.ForeColor = TextPri;
        _gfx.Items.AddRange(new object[] { "D3D10", "D3D11", "Vulkan" });
        _gfx.SelectedItem = _settings.GraphicsBackend;
        if (_gfx.SelectedIndex < 0) _gfx.SelectedIndex = 1;

        var thrLbl = new Label { Text = "CPU threads", ForeColor = TextSec, Location = new Point(0, 56), AutoSize = true };
        _threads.Minimum = 1;
        _threads.Maximum = 64;
        _threads.Width = 80;
        _threads.Location = new Point(0, 74);
        _threads.BackColor = Panel2;
        _threads.ForeColor = TextPri;
        _threads.Value = Math.Clamp(_settings.PpuThreads, 1, 64);

        optsPanel.Controls.Add(gfxLbl);
        optsPanel.Controls.Add(_gfx);
        optsPanel.Controls.Add(thrLbl);
        optsPanel.Controls.Add(_threads);

        rail.Controls.Add(optsPanel);
        rail.Controls.Add(optsTitle);
        rail.Controls.Add(btnStack);
        rail.Controls.Add(railTitle);

        // ---- Status bar ----
        var statusBar = new Panel { Dock = DockStyle.Bottom, Height = 32, BackColor = Panel, Padding = new Padding(12, 0, 12, 0) };
        _status.Text = "Ready";
        _status.ForeColor = TextSec;
        _status.AutoSize = true;
        _status.Location = new Point(12, 8);
        _progress.Style = ProgressBarStyle.Marquee;
        _progress.MarqueeAnimationSpeed = 0;
        _progress.Width = 180;
        _progress.Height = 12;
        _progress.Location = new Point(Width - 220, 10);
        _progress.Anchor = AnchorStyles.Top | AnchorStyles.Right;
        statusBar.Controls.Add(_status);
        statusBar.Controls.Add(_progress);

        // ---- Project strip ----
        var projStrip = new Panel { Dock = DockStyle.Top, Height = 36, BackColor = Panel2, Padding = new Padding(12, 0, 12, 0) };
        _project.Text = "No project loaded";
        _project.ForeColor = TextSec;
        _project.AutoSize = true;
        _project.Location = new Point(12, 10);
        projStrip.Controls.Add(_project);

        // ---- Tabs / content ----
        var tabs = new TabControl { Dock = DockStyle.Fill, Padding = new Point(12, 8) };
        tabs.Font = new Font("Segoe UI", 9f);

        StyleTextBox(_log);
        StyleTextBox(_report);

        var logTab = new TabPage("Console");
        logTab.BackColor = Bg;
        logTab.Controls.Add(_log);

        var reportTab = new TabPage("Lift report");
        reportTab.BackColor = Bg;
        reportTab.Controls.Add(_report);

        var roadmapTab = new TabPage("Roadmap");
        roadmapTab.BackColor = Bg;
        var roadmapBox = new TextBox();
        StyleTextBox(roadmapBox);
        roadmapBox.Text = RoadmapText;
        roadmapTab.Controls.Add(roadmapBox);

        tabs.TabPages.Add(logTab);
        tabs.TabPages.Add(reportTab);
        tabs.TabPages.Add(roadmapTab);

        Controls.Add(tabs);
        Controls.Add(projStrip);
        Controls.Add(statusBar);
        Controls.Add(rail);
        Controls.Add(header);

        _btnDecrypt.Click += async (_, _) => await DecryptEbootAndLoad();
        _btnLoad.Click += (_, _) => LoadElf(null);
        _btnLift.Click += async (_, _) => await RunStep("Decompiling…", ct =>
        {
            ct.ThrowIfCancellationRequested();
            bool ok = Native.Lift(_projectDir!, out var l);
            return (ok, l);
        }, enableBuild: true, refreshReport: true);
        _btnBuild.Click += async (_, _) => await RunStep("Building…", ct =>
        {
            ct.ThrowIfCancellationRequested();
            bool ok = Native.Build(_projectDir!, RuntimeDir, CompilersDir, out var l);
            return (ok, l);
        }, enableCopy: true);
        _btnCopy.Click += (_, _) => CopyToEbootFolder();
        _btnCancel.Click += (_, _) => { try { _cts?.Cancel(); } catch { } Append("Cancel requested…"); };
        FormClosing += (_, _) => SaveSettings();

        Append($"ps3core {SafeVersion()}  ·  runtime {RuntimeDir}");
        Append("Decrypt EBOOT uses: rpcs3.exe --decrypt \"EBOOT.BIN\"  (Utilities → Decrypt PS3 Binaries).");
        Append("Generated .elf is loaded automatically into a new project.");
    }

    private static void StyleTextBox(TextBox t)
    {
        t.Multiline = true;
        t.ReadOnly = true;
        t.ScrollBars = ScrollBars.Both;
        t.Dock = DockStyle.Fill;
        t.BorderStyle = BorderStyle.None;
        t.BackColor = Bg;
        t.ForeColor = TextPri;
        t.Font = new Font("Consolas", 9.25f);
        t.WordWrap = false;
    }

    private static void StylePrimaryButton(Button b, string text)
    {
        b.Text = text;
        b.FlatStyle = FlatStyle.Flat;
        b.FlatAppearance.BorderSize = 0;
        b.BackColor = Accent;
        b.ForeColor = Color.White;
        b.Height = 36;
        b.Cursor = Cursors.Hand;
        b.Font = new Font("Segoe UI Semibold", 9.25f);
        b.MouseEnter += (_, _) => { if (b.Enabled) b.BackColor = AccentHover; };
        b.MouseLeave += (_, _) => { if (b.Enabled) b.BackColor = Accent; };
    }

    private static void StyleStepButton(Button b, string text)
    {
        b.Text = text;
        b.FlatStyle = FlatStyle.Flat;
        b.FlatAppearance.BorderColor = Border;
        b.FlatAppearance.BorderSize = 1;
        b.BackColor = Panel2;
        b.ForeColor = TextPri;
        b.Height = 34;
        b.Cursor = Cursors.Hand;
        b.MouseEnter += (_, _) => { if (b.Enabled) b.BackColor = Color.FromArgb(48, 48, 56); };
        b.MouseLeave += (_, _) => { if (b.Enabled) b.BackColor = Panel2; };
    }

    private static void StyleDangerButton(Button b, string text)
    {
        b.Text = text;
        b.FlatStyle = FlatStyle.Flat;
        b.FlatAppearance.BorderSize = 0;
        b.BackColor = Color.FromArgb(60, 40, 40);
        b.ForeColor = Color.FromArgb(255, 160, 160);
        b.Height = 32;
        b.Cursor = Cursors.Hand;
    }

    private static string SafeVersion()
    {
        try { return Native.Version().ToString(); }
        catch (Exception ex) { return "UNAVAILABLE (" + ex.Message + ")"; }
    }

    private void Append(string s)
    {
        if (InvokeRequired) { BeginInvoke(() => Append(s)); return; }
        _log.AppendText(s.Replace("\n", Environment.NewLine) + Environment.NewLine);
    }

    private void SetStatus(string s)
    {
        if (InvokeRequired) { BeginInvoke(() => SetStatus(s)); return; }
        _status.Text = s;
        _status.ForeColor = s.Contains("fail", StringComparison.OrdinalIgnoreCase) || s.Contains("Error")
            ? Danger
            : TextSec;
    }

    private void SaveSettings()
    {
        _settings.GraphicsBackend = _gfx.SelectedItem?.ToString() ?? "D3D11";
        _settings.PpuThreads = (int)_threads.Value;
        _settings.Save();
    }

    private void LoadElf(string? presetPath)
    {
        string? path = presetPath;
        if (path == null)
        {
            using var ofd = new OpenFileDialog
            {
                Title = "Select decrypted PS3 ELF",
                Filter = "ELF|*.elf;*.ELF|All files|*.*"
            };
            if (ofd.ShowDialog(this) != DialogResult.OK) return;
            path = ofd.FileName;
        }

        using var dlg = new PromptDialog("Project name", "Game / project folder name:");
        if (dlg.ShowDialog(this) != DialogResult.OK || dlg.Value.Length == 0) return;

        if (!Native.CreateProject(AppContext.BaseDirectory, dlg.Value, path, out var dir, out var err))
        {
            Append("ERROR: " + err);
            return;
        }
        _projectDir = dir;
        _project.Text = "Project  ·  " + dir;
        _project.ForeColor = TextPri;
        Append("Created project: " + dir);
        _canBuild = false;
        _canCopy = false;
        _btnLift.Enabled = true;
        _btnBuild.Enabled = false;
        _btnCopy.Enabled = false;
        _report.Text = "Run Decompile to generate lift_report.txt";
    }

    private async Task DecryptEbootAndLoad()
    {
        using var ofd = new OpenFileDialog
        {
            Title = "Select EBOOT.BIN (or SELF) to decrypt",
            Filter = "EBOOT.BIN|EBOOT.BIN|SELF / BIN|*.self;*.bin;*.sprx|All files|*.*"
        };
        if (ofd.ShowDialog(this) != DialogResult.OK) return;

        if (string.IsNullOrEmpty(_settings.Rpcs3Path) || !File.Exists(_settings.Rpcs3Path))
        {
            using var rp = new OpenFileDialog { Title = "Locate rpcs3.exe", Filter = "rpcs3.exe|rpcs3.exe" };
            if (rp.ShowDialog(this) != DialogResult.OK) return;
            _settings.Rpcs3Path = rp.FileName;
            SaveSettings();
        }

        string bin = ofd.FileName;
        Append("Decrypting: " + bin);
        SetBusy(true);
        SetStatus("Decrypting…");
        _progress.MarqueeAnimationSpeed = 30;

        string elf = "";
        string err = "";
        bool ok = false;
        try
        {
            ok = await Task.Run(() => Rpcs3Launcher.DecryptBinary(_settings.Rpcs3Path, bin, out elf, out err));
        }
        catch (Exception ex)
        {
            err = ex.Message;
            ok = false;
        }
        finally
        {
            _progress.MarqueeAnimationSpeed = 0;
            SetBusy(false);
            SetStatus(ok ? "Ready" : "Decrypt failed");
        }

        if (!ok)
        {
            Append("ERROR: " + err);
            return;
        }

        Append("Decrypted ELF: " + elf);
        LoadElf(elf);
    }

    private async Task RunStep(string title, Func<CancellationToken, (bool ok, string log)> work, bool enableBuild = false, bool enableCopy = false, bool refreshReport = false)
    {
        if (_projectDir == null) return;
        SaveSettings();
        Append(title);
        _cts?.Dispose();
        _cts = new CancellationTokenSource();
        SetBusy(true);
        SetStatus(title);
        _progress.MarqueeAnimationSpeed = 30;
        try
        {
            var token = _cts.Token;
            var (ok, log) = await Task.Run(() => work(token), token);
            Append(log);
            Append(ok ? "DONE" : "FAILED");
            if (ok && enableBuild) _canBuild = true;
            if (ok && enableCopy) _canCopy = true;
            if (refreshReport) LoadLiftReport();
            SetStatus(ok ? "Ready" : "Failed");
        }
        catch (OperationCanceledException)
        {
            Append("CANCELLED");
            SetStatus("Cancelled");
        }
        catch (Exception ex)
        {
            Append("ERROR: " + ex.Message);
            SetStatus("Error");
        }
        finally
        {
            _progress.MarqueeAnimationSpeed = 0;
            SetBusy(false);
        }
    }

    private void LoadLiftReport()
    {
        if (_projectDir == null) return;
        string[] candidates =
        {
            Path.Combine(_projectDir, "codebase", "lift_report.txt"),
            Path.Combine(_projectDir, "output", "lift_report.txt"),
            Path.Combine(_projectDir, "lift_report.txt"),
            Path.Combine(_projectDir, "analysis_report.txt"),
        };
        foreach (var p in candidates)
        {
            if (!File.Exists(p)) continue;
            try
            {
                _report.Text = File.ReadAllText(p);
                Append("Report loaded: " + p);
                return;
            }
            catch (Exception ex) { Append("Could not read report: " + ex.Message); }
        }
        _report.Text = "(lift_report.txt not found yet)";
    }

    private void SetBusy(bool busy)
    {
        _btnDecrypt.Enabled = !busy;
        _btnLoad.Enabled = !busy;
        _btnLift.Enabled = !busy && _projectDir != null;
        _btnBuild.Enabled = !busy && _canBuild;
        _btnCopy.Enabled = !busy && _canCopy;
        _btnCancel.Enabled = busy;
        UseWaitCursor = busy;
    }

    private void CopyToEbootFolder()
    {
        if (_projectDir == null) return;
        using var fbd = new FolderBrowserDialog { Description = "Folder that contains EBOOT.BIN (…/PS3_GAME/USRDIR)" };
        if (fbd.ShowDialog(this) != DialogResult.OK) return;
        string outDir = Path.Combine(_projectDir, "output");
        foreach (var name in new[] { "game.exe", "ps3rt.dll", "guest_image.bin" })
        {
            string src = Path.Combine(outDir, name);
            if (File.Exists(src))
            {
                File.Copy(src, Path.Combine(fbd.SelectedPath, name), true);
                Append("Copied " + name);
            }
            else Append("Missing: " + name);
        }
        Append("Run game.exe from that folder.");
    }

    private const string RoadmapText =
        "PPSX33 Roadmap (summary)\r\n\r\n" +
        "Phase 1  UI              DONE  (dark studio UI, RPCS3 --decrypt)\r\n" +
        "Phase 2  PPU lift        GOW3 + Uncharted2 static 100% snapshots\r\n" +
        "Phase 3  PPU runtime     IN PROGRESS  (external stubs, HLE)\r\n" +
        "Phase 4  SPU             Interpreter (major ISA)\r\n" +
        "Phase 5  RSX             GCM/FIFO core started\r\n" +
        "Phase 6  Native output   DONE  (MSVC game.exe + ps3rt.dll)\r\n\r\n" +
        "See ROADMAP.md in the repository for full detail.\r\n";
}
