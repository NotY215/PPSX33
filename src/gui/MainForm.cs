namespace PS3Recomp.Gui;

/// <summary>
/// Phase 1 UI. Workflow:
///   1 Load ELF -> ask game name -> create project folders
///   2 Decompile (lift PPU -> C++)
///   3 Build (MSVC from Compilers-files)
///   4 Copy result next to EBOOT.BIN
/// </summary>
public sealed class MainForm : Form
{
    private readonly Settings _settings = Settings.Load();
    private readonly TextBox _log = new() { Multiline = true, ReadOnly = true, ScrollBars = ScrollBars.Both, Dock = DockStyle.Fill, Font = new Font("Consolas", 9f), WordWrap = false };
    private readonly TextBox _report = new() { Multiline = true, ReadOnly = true, ScrollBars = ScrollBars.Both, Dock = DockStyle.Fill, Font = new Font("Consolas", 9f), WordWrap = false };
    private readonly Label _project = new() { Text = "No project loaded", AutoSize = true };
    private readonly Label _status = new() { Text = "Ready", AutoSize = true, Padding = new Padding(12, 6, 0, 0) };
    private readonly ProgressBar _progress = new() { Width = 220, Style = ProgressBarStyle.Marquee, MarqueeAnimationSpeed = 0, Visible = true };
    private readonly Button _btnLoad = new() { Text = "1. Load ELF…", AutoSize = true };
    private readonly Button _btnFindElf = new() { Text = "Find decrypted ELF…", AutoSize = true };
    private readonly Button _btnLift = new() { Text = "2. Decompile / Recompile to C++", AutoSize = true, Enabled = false };
    private readonly Button _btnBuild = new() { Text = "3. Build exe + dll", AutoSize = true, Enabled = false };
    private readonly Button _btnCopy = new() { Text = "4. Copy to EBOOT folder…", AutoSize = true, Enabled = false };
    private readonly Button _btnCancel = new() { Text = "Cancel", AutoSize = true, Enabled = false };
    private readonly Button _btnRpcs3 = new() { Text = "Open EBOOT in RPCS3 (decrypt)…", AutoSize = true };
    private readonly ComboBox _gfx = new() { DropDownStyle = ComboBoxStyle.DropDownList, Width = 90 };
    private readonly NumericUpDown _threads = new() { Minimum = 1, Maximum = 64, Width = 50 };
    private string? _projectDir;
    private CancellationTokenSource? _cts;
    private bool _canBuild;
    private bool _canCopy;

    private static string RuntimeDir => Path.Combine(AppContext.BaseDirectory, "runtime");
    private static string CompilersDir => Path.Combine(AppContext.BaseDirectory, "Compilers-files");

    public MainForm()
    {
        Text = "PS3 Recompiler"; Width = 960; Height = 680; StartPosition = FormStartPosition.CenterScreen;

        _gfx.Items.AddRange(new object[] { "D3D10", "D3D11", "Vulkan" });
        _gfx.SelectedItem = _settings.GraphicsBackend; if (_gfx.SelectedIndex < 0) _gfx.SelectedIndex = 1;
        _threads.Value = Math.Clamp(_settings.PpuThreads, 1, 64);

        var top = new FlowLayoutPanel { Dock = DockStyle.Top, AutoSize = true, Padding = new Padding(6) };
        top.Controls.AddRange(new Control[] { _btnLoad, _btnFindElf, _btnLift, _btnBuild, _btnCopy, _btnCancel });
        var opts = new FlowLayoutPanel { Dock = DockStyle.Top, AutoSize = true, Padding = new Padding(6) };
        opts.Controls.AddRange(new Control[] {
            new Label { Text = "Graphics:", AutoSize = true, Padding = new Padding(0, 6, 0, 0) }, _gfx,
            new Label { Text = "CPU threads:", AutoSize = true, Padding = new Padding(12, 6, 0, 0) }, _threads, _btnRpcs3, _status, _progress });
        var info = new FlowLayoutPanel { Dock = DockStyle.Top, AutoSize = true, Padding = new Padding(6) };
        info.Controls.Add(_project);

        var tabs = new TabControl { Dock = DockStyle.Fill };
        var logTab = new TabPage("Log"); logTab.Controls.Add(_log);
        var reportTab = new TabPage("Lift report"); reportTab.Controls.Add(_report);
        var roadmapTab = new TabPage("Roadmap");
        roadmapTab.Controls.Add(new TextBox { Multiline = true, ReadOnly = true, Dock = DockStyle.Fill, ScrollBars = ScrollBars.Vertical, Text = RoadmapText });
        tabs.TabPages.Add(logTab);
        tabs.TabPages.Add(reportTab);
        tabs.TabPages.Add(roadmapTab);

        Controls.Add(tabs); Controls.Add(info); Controls.Add(opts); Controls.Add(top);

        _btnLoad.Click += (_, _) => LoadElf(null);
        _btnFindElf.Click += (_, _) => FindAndLoadDecryptedElf();
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
        _btnRpcs3.Click += (_, _) => OpenRpcs3();
        FormClosing += (_, _) => SaveSettings();
        Append($"ps3core version {SafeVersion()}  |  runtime: {RuntimeDir}  |  compilers: {CompilersDir}");
        Append("Tip: use 'Find decrypted ELF' after RPCS3 has opened the game once, or Load ELF manually.");
    }

    private static string SafeVersion() { try { return Native.Version().ToString(); } catch (Exception ex) { return "UNAVAILABLE (" + ex.Message + ")"; } }

    private void Append(string s)
    {
        if (InvokeRequired) { BeginInvoke(() => Append(s)); return; }
        _log.AppendText(s.Replace("\n", Environment.NewLine) + Environment.NewLine);
    }

    private void SetStatus(string s)
    {
        if (InvokeRequired) { BeginInvoke(() => SetStatus(s)); return; }
        _status.Text = s;
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
            using var ofd = new OpenFileDialog { Title = "Select decrypted PS3 ELF", Filter = "ELF / EBOOT|*.elf;*.self;EBOOT.*|All files|*.*" };
            if (ofd.ShowDialog(this) != DialogResult.OK) return;
            path = ofd.FileName;
        }

        using var dlg = new PromptDialog("Game name", "Enter the game name (a folder with this name is created next to the app):");
        if (dlg.ShowDialog(this) != DialogResult.OK || dlg.Value.Length == 0) return;

        if (!Native.CreateProject(AppContext.BaseDirectory, dlg.Value, path, out var dir, out var err))
        { Append("ERROR: " + err); return; }
        _projectDir = dir;
        _project.Text = "Project: " + dir;
        Append($"Created project folders in {dir}\n  input/ (ELF copied)  codebase/  output/");
        _canBuild = false;
        _canCopy = false;
        _btnLift.Enabled = true;
        _btnBuild.Enabled = false;
        _btnCopy.Enabled = false;
        _report.Text = "(Run Decompile to generate lift_report.txt)";
    }

    private void FindAndLoadDecryptedElf()
    {
        using var ofd = new OpenFileDialog { Title = "Select original EBOOT.BIN (used as search root)", Filter = "EBOOT.BIN|EBOOT.BIN|All files|*.*" };
        string? eboot = null;
        if (ofd.ShowDialog(this) == DialogResult.OK) eboot = ofd.FileName;

        Append("Searching for decrypted ELF…");
        var hits = Rpcs3Launcher.FindDecryptedElfs(_settings.Rpcs3Path, eboot);
        if (hits.Count == 0)
        {
            Append("No decrypted ELF found. Open the game once in RPCS3, or browse manually with Load ELF.");
            return;
        }

        using var pick = new Form
        {
            Text = "Select decrypted ELF",
            Width = 720,
            Height = 360,
            StartPosition = FormStartPosition.CenterParent
        };
        var list = new ListBox { Dock = DockStyle.Fill };
        foreach (var h in hits) list.Items.Add(h);
        list.SelectedIndex = 0;
        var ok = new Button { Text = "Use selected", Dock = DockStyle.Bottom, Height = 32 };
        ok.Click += (_, _) => { pick.DialogResult = DialogResult.OK; pick.Close(); };
        pick.Controls.Add(list);
        pick.Controls.Add(ok);
        if (pick.ShowDialog(this) != DialogResult.OK || list.SelectedItem is not string chosen) return;
        Append("Using: " + chosen);
        LoadElf(chosen);
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
            Path.Combine(_projectDir, "codebase", "analysis_report.txt"),
            Path.Combine(_projectDir, "output", "analysis_report.txt"),
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
        _btnLoad.Enabled = !busy;
        _btnFindElf.Enabled = !busy;
        _btnLift.Enabled = !busy && _projectDir != null;
        _btnBuild.Enabled = !busy && _canBuild;
        _btnCopy.Enabled = !busy && _canCopy;
        _btnCancel.Enabled = busy;
        _btnRpcs3.Enabled = !busy;
        UseWaitCursor = busy;
    }

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
        Append("Run game.exe from that folder (native recompile; not for RPCS3).");
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
        else Append("RPCS3 started. After it decrypts, use 'Find decrypted ELF' or Load ELF manually.");
    }

    private const string RoadmapText =
        "Phase 1  UI                          - DONE (progress, cancel, lift report, ELF discovery)\r\n" +
        "Phase 2  ELF decompile               - DONE for GOW3 static coverage; OPD/SPU extract added\r\n" +
        "Phase 3  PPU runtime                 - IN PROGRESS (memory, external PC stubs, syscalls)\r\n" +
        "Phase 4  SPU                         - IN PROGRESS (skeleton: mbox/MFC/run stub)\r\n" +
        "Phase 5  RSX graphics                - TODO\r\n" +
        "Phase 6  Native output               - DONE (MSVC game.exe + ps3rt.dll)\r\n\r\n" +
        "Details: ROADMAP.md in the repository.";
}
