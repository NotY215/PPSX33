namespace PS3Recomp.Gui;

/// <summary>
/// Phase 1 UI. Workflow:
///   1 Load ELF -> ask game name -> create &lt;exe dir&gt;/&lt;game&gt;/{input,codebase,output}
///   2 Decompile (lift PPU -> C++)      3 Build (g++/ninja from Compilers-files)
///   4 Copy result next to EBOOT.BIN
/// </summary>
public sealed class MainForm : Form
{
    private readonly Settings _settings = Settings.Load();
    private readonly TextBox _log = new() { Multiline = true, ReadOnly = true, ScrollBars = ScrollBars.Both, Dock = DockStyle.Fill, Font = new Font("Consolas", 9f), WordWrap = false };
    private readonly Label _project = new() { Text = "No project loaded", AutoSize = true };
    private readonly Button _btnLoad = new() { Text = "1. Load ELF…", AutoSize = true };
    private readonly Button _btnLift = new() { Text = "2. Decompile / Recompile to C++", AutoSize = true, Enabled = false };
    private readonly Button _btnBuild = new() { Text = "3. Build exe + dll", AutoSize = true, Enabled = false };
    private readonly Button _btnCopy = new() { Text = "4. Copy to EBOOT folder…", AutoSize = true, Enabled = false };
    private readonly Button _btnRpcs3 = new() { Text = "Open EBOOT in RPCS3 (decrypt)…", AutoSize = true };
    private readonly ComboBox _gfx = new() { DropDownStyle = ComboBoxStyle.DropDownList, Width = 90 };
    private readonly NumericUpDown _threads = new() { Minimum = 1, Maximum = 64, Width = 50 };
    private string? _projectDir;

    private static string RuntimeDir => Path.Combine(AppContext.BaseDirectory, "runtime");
    private static string CompilersDir => Path.Combine(AppContext.BaseDirectory, "Compilers-files");

    public MainForm()
    {
        Text = "PS3 Recompiler"; Width = 900; Height = 640; StartPosition = FormStartPosition.CenterScreen;

        _gfx.Items.AddRange(new object[] { "D3D10", "D3D11", "Vulkan" });
        _gfx.SelectedItem = _settings.GraphicsBackend; if (_gfx.SelectedIndex < 0) _gfx.SelectedIndex = 1;
        _threads.Value = Math.Clamp(_settings.PpuThreads, 1, 64);

        var top = new FlowLayoutPanel { Dock = DockStyle.Top, AutoSize = true, Padding = new Padding(6) };
        top.Controls.AddRange(new Control[] { _btnLoad, _btnLift, _btnBuild, _btnCopy });
        var opts = new FlowLayoutPanel { Dock = DockStyle.Top, AutoSize = true, Padding = new Padding(6) };
        opts.Controls.AddRange(new Control[] {
            new Label { Text = "Graphics:", AutoSize = true, Padding = new Padding(0, 6, 0, 0) }, _gfx,
            new Label { Text = "CPU threads:", AutoSize = true, Padding = new Padding(12, 6, 0, 0) }, _threads, _btnRpcs3 });
        var info = new FlowLayoutPanel { Dock = DockStyle.Top, AutoSize = true, Padding = new Padding(6) };
        info.Controls.Add(_project);

        var tabs = new TabControl { Dock = DockStyle.Fill };
        var logTab = new TabPage("Log"); logTab.Controls.Add(_log);
        var roadmapTab = new TabPage("Roadmap");
        roadmapTab.Controls.Add(new TextBox { Multiline = true, ReadOnly = true, Dock = DockStyle.Fill, ScrollBars = ScrollBars.Vertical, Text = RoadmapText });
        tabs.TabPages.Add(logTab); tabs.TabPages.Add(roadmapTab);

        Controls.Add(tabs); Controls.Add(info); Controls.Add(opts); Controls.Add(top);

        _btnLoad.Click += (_, _) => LoadElf();
        _btnLift.Click += async (_, _) => await RunStep("Decompiling…", () => { bool ok = Native.Lift(_projectDir!, out var l); return (ok, l); }, enableBuild: true);
        _btnBuild.Click += async (_, _) => await RunStep("Building…", () => { bool ok = Native.Build(_projectDir!, RuntimeDir, CompilersDir, out var l); return (ok, l); }, enableCopy: true);
        _btnCopy.Click += (_, _) => CopyToEbootFolder();
        _btnRpcs3.Click += (_, _) => OpenRpcs3();
        FormClosing += (_, _) => SaveSettings();
        Append($"ps3core version {SafeVersion()}  |  runtime: {RuntimeDir}  |  compilers: {CompilersDir}");
    }

    private static string SafeVersion() { try { return Native.Version().ToString(); } catch (Exception ex) { return "UNAVAILABLE (" + ex.Message + ")"; } }

    private void Append(string s) => _log.AppendText(s.Replace("\n", Environment.NewLine) + Environment.NewLine);

    private void SaveSettings()
    {
        _settings.GraphicsBackend = _gfx.SelectedItem?.ToString() ?? "D3D11";
        _settings.PpuThreads = (int)_threads.Value;
        _settings.Save();
    }

    private void LoadElf()
    {
        using var ofd = new OpenFileDialog { Title = "Select decrypted PS3 ELF", Filter = "ELF / EBOOT|*.elf;*.self;EBOOT.*|All files|*.*" };
        if (ofd.ShowDialog(this) != DialogResult.OK) return;

        using var dlg = new PromptDialog("Game name", "Enter the game name (a folder with this name is created next to the app):");
        if (dlg.ShowDialog(this) != DialogResult.OK || dlg.Value.Length == 0) return;

        if (!Native.CreateProject(AppContext.BaseDirectory, dlg.Value, ofd.FileName, out var dir, out var err))
        { Append("ERROR: " + err); return; }
        _projectDir = dir;
        _project.Text = "Project: " + dir;
        Append($"Created project folders in {dir}\n  input/ (ELF copied)  codebase/  output/");
        _btnLift.Enabled = true; _btnBuild.Enabled = false; _btnCopy.Enabled = false;
    }

    private async Task RunStep(string title, Func<(bool ok, string log)> work, bool enableBuild = false, bool enableCopy = false)
    {
        if (_projectDir == null) return;
        SaveSettings();
        Append(title); SetBusy(true);
        var (ok, log) = await Task.Run(work);
        Append(log); Append(ok ? "DONE" : "FAILED");
        SetBusy(false);
        if (ok && enableBuild) _btnBuild.Enabled = true;
        if (ok && enableCopy) _btnCopy.Enabled = true;
    }

    private void SetBusy(bool busy) { foreach (var b in new[] { _btnLoad, _btnLift, _btnBuild, _btnCopy }) b.Enabled = !busy && (b == _btnLoad || _projectDir != null); UseWaitCursor = busy; }

    private void CopyToEbootFolder()
    {
        if (_projectDir == null) return;
        using var fbd = new FolderBrowserDialog { Description = "Select the folder that contains EBOOT.BIN (…/PS3_GAME/USRDIR)" };
        if (fbd.ShowDialog(this) != DialogResult.OK) return;
        string outDir = Path.Combine(_projectDir, "output");
        foreach (var name in new[] { "game.exe", "ps3rt.dll", "guest_image.bin" })
        {
            string src = Path.Combine(outDir, name);
            if (File.Exists(src)) { File.Copy(src, Path.Combine(fbd.SelectedPath, name), true); Append("Copied " + name); }
            else Append("Missing (not built?): " + name);
        }
        Append("Run game.exe from that folder.");
    }

    private void OpenRpcs3()
    {
        using var ofd = new OpenFileDialog { Title = "Select EBOOT.BIN to open in RPCS3", Filter = "EBOOT.BIN|EBOOT.BIN|All files|*.*" };
        if (ofd.ShowDialog(this) != DialogResult.OK) return;
        if (string.IsNullOrEmpty(_settings.Rpcs3Path) || !File.Exists(_settings.Rpcs3Path))
        {
            using var rp = new OpenFileDialog { Title = "Locate rpcs3.exe", Filter = "rpcs3.exe|rpcs3.exe" };
            if (rp.ShowDialog(this) != DialogResult.OK) return;
            _settings.Rpcs3Path = rp.FileName;
        }
        if (!Rpcs3Launcher.Launch(_settings.Rpcs3Path, ofd.FileName, out var err)) Append("ERROR: " + err);
        else Append("RPCS3 started. Use it only to obtain a decrypted ELF, then press 'Load ELF…'.");
    }

    private const string RoadmapText =
        "Phase 1  UI                          - DONE (this window)\r\n" +
        "Phase 2  ELF decompile               - IN PROGRESS (loader + PPU lifter subset)\r\n" +
        "Phase 3  PPU -> x86-64 (4c/4t)       - IN PROGRESS (single-thread baseline works)\r\n" +
        "Phase 4  SPU                         - TODO\r\n" +
        "Phase 5  RSX -> D3D10 / D3D11 / Vulkan - TODO\r\n" +
        "Phase 6  Auto build exe + dll        - IN PROGRESS (g++/ninja driver works)\r\n\r\n" +
        "Details: README.md and ROADMAP.md in the repository.";
}
