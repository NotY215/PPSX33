using System.Diagnostics;
using System.Runtime.InteropServices;
using System.Text;

namespace PS3Recomp.Gui;

/// <summary>Static dark WinForms shell. Load ELF → Decompile → Build → Run.</summary>
public sealed partial class MainForm : Form
{
    static readonly Color Bg = Color.FromArgb(18, 18, 22);
    static readonly Color Panel = Color.FromArgb(28, 28, 34);
    static readonly Color Panel2 = Color.FromArgb(36, 36, 44);
    static readonly Color TextPri = Color.FromArgb(230, 232, 240);
    static readonly Color TextSec = Color.FromArgb(140, 145, 160);
    static readonly Color Accent = Color.FromArgb(70, 130, 255);
    static readonly Color Danger = Color.FromArgb(220, 80, 90);

    private readonly Settings _settings = Settings.Load();
    private readonly TextBox _log = new();
    private readonly TextBox _report = new();
    private readonly TextBox _roadmap = new();
    private readonly Label _project = new();
    private readonly Label _status = new();
    private readonly ProgressBar _progress = new();
    private readonly Button _btnDecrypt = new();
    private readonly Button _btnLoad = new();
    private readonly Button _btnLift = new();
    private readonly Button _btnBuild = new();
    private readonly Button _btnRun = new();
    private readonly Button _btnCopy = new();
    private readonly Button _btnCancel = new();
    private readonly ComboBox _gfx = new();
    private readonly NumericUpDown _threads = new();
    private readonly TabControl _tabs = new();
    private readonly PictureBox _logo = new();
    private string? _projectDir;
    private CancellationTokenSource? _cts;
    private Process? _gameProc;
    private bool _canLift, _canBuild, _canCopy, _canRun, _gfxUnlocked, _busy;
    private int _liftChunks;
    private Button? _btnRpcs3;
    private readonly Label _eta = new();

    private const int StallQuietMs = 4000;
    private const int MaxRunMs = 120_000;
    private static string RuntimeDir => Path.Combine(AppContext.BaseDirectory, "runtime");
    private static string CompilersDir => Path.Combine(AppContext.BaseDirectory, "Compilers-files");
    private static string ProjectsRoot => Path.Combine(AppContext.BaseDirectory, "projects");

    [DllImport("dwmapi.dll")]
    private static extern int DwmSetWindowAttribute(IntPtr hwnd, int attr, ref int value, int size);

    public MainForm()
    {
        AutoScaleMode = AutoScaleMode.Dpi;
        Text = "PPSX33  ·  PlayStation 3 Recompiler";
        Width = 1120; Height = 740;
        MinimumSize = new Size(960, 640);
        StartPosition = FormStartPosition.CenterScreen;
        BackColor = Bg; ForeColor = TextPri;
        Font = new Font("Segoe UI", 9.25f);
        DoubleBuffered = true;
        try {
            string ico = Path.Combine(AppContext.BaseDirectory, "ppsx33-logo.ico");
            if (!File.Exists(ico)) ico = Path.Combine(AppContext.BaseDirectory, "assets", "ppsx33-logo.ico");
            if (File.Exists(ico)) Icon = new Icon(ico);
        } catch { }

        var root = new TableLayoutPanel { Dock = DockStyle.Fill, ColumnCount = 2, RowCount = 1, Padding = new Padding(10), BackColor = Bg };
        root.ColumnStyles.Add(new ColumnStyle(SizeType.Absolute, 250));
        root.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 100f));
        var rail = new Panel { Dock = DockStyle.Fill, BackColor = Panel, Padding = new Padding(12) };
        _logo.SizeMode = PictureBoxSizeMode.Zoom; _logo.Height = 56; _logo.Dock = DockStyle.Top; _logo.BackColor = Color.Transparent;
        try {
            string png = Path.Combine(AppContext.BaseDirectory, "ppsx33-logo.png");
            if (!File.Exists(png)) png = Path.Combine(AppContext.BaseDirectory, "assets", "ppsx33-logo.png");
            if (File.Exists(png)) _logo.Image = Image.FromFile(png);
        } catch { }

        var railTitle = MakeLbl("WORKFLOW", TextSec, true); railTitle.Dock = DockStyle.Top; railTitle.Height = 24;
        StyleBtn(_btnDecrypt, "Decrypt EBOOT", Accent);
        StyleBtn(_btnLoad, "Load ELF", Panel2);
        StyleBtn(_btnLift, "Decompile", Panel2);
        StyleBtn(_btnBuild, "Build", Panel2);
        StyleBtn(_btnRun, "Run game.exe", Accent);
        StyleBtn(_btnCopy, "Copy to game folder", Panel2);
        StyleBtn(_btnCancel, "Stop / Cancel", Danger);
        _btnLift.Enabled = _btnBuild.Enabled = _btnRun.Enabled = _btnCopy.Enabled = _btnCancel.Enabled = false;
        var btnStack = new FlowLayoutPanel { Dock = DockStyle.Top, FlowDirection = FlowDirection.TopDown, WrapContents = false, AutoSize = true, BackColor = Color.Transparent, Padding = new Padding(0, 4, 0, 8) };
        foreach (var b in new[] { _btnDecrypt, _btnLoad, _btnLift, _btnBuild, _btnRun, _btnCopy, _btnCancel })
        { b.Width = 214; b.Height = 34; b.Margin = new Padding(0, 0, 0, 6); btnStack.Controls.Add(b); }

        var optsTitle = MakeLbl("OPTIONS", TextSec, true); optsTitle.Dock = DockStyle.Top; optsTitle.Height = 28; optsTitle.Padding = new Padding(0, 8, 0, 0);
        var opts = new Panel { Dock = DockStyle.Top, Height = 170, BackColor = Color.Transparent };
        var gfxLbl = MakeLbl("Graphics backend", TextSec, false); gfxLbl.Location = new Point(0, 4);
        _gfx.DropDownStyle = ComboBoxStyle.DropDownList; _gfx.FlatStyle = FlatStyle.Flat;
        _gfx.BackColor = Panel2; _gfx.ForeColor = TextPri; _gfx.Location = new Point(0, 24); _gfx.Size = new Size(214, 28);
        _gfx.Items.AddRange(new object[] { "D3D10", "D3D11" });
        _gfx.SelectedItem = _settings.GraphicsBackend; if (_gfx.SelectedIndex < 0) _gfx.SelectedIndex = 1;
        _gfx.Enabled = false;
        var thrLbl = MakeLbl("CPU threads", TextSec, false); thrLbl.Location = new Point(0, 58);
        _threads.Minimum = 1; _threads.Maximum = 64; _threads.BackColor = Panel2; _threads.ForeColor = TextPri;
        _threads.Location = new Point(0, 78); _threads.Size = new Size(100, 26);
        _threads.Value = Math.Clamp(_settings.PpuThreads, 1, 64);
        var rpcs3Lbl = MakeLbl("RPCS3 (decrypt)", TextSec, false); rpcs3Lbl.Location = new Point(0, 108);
        _btnRpcs3 = new Button { Text = "Choose rpcs3.exe…", FlatStyle = FlatStyle.Flat, BackColor = Panel2, ForeColor = TextPri, Location = new Point(0, 128), Size = new Size(214, 28), Cursor = Cursors.Hand };
        _btnRpcs3.FlatAppearance.BorderSize = 0;
        _btnRpcs3.Click += (_, _) => { if (PickRpcs3Exe()) { Append("RPCS3: " + _settings.Rpcs3Path); SaveSettings(); UpdateRpcs3Button(); } };
        opts.Controls.AddRange(new Control[] { gfxLbl, _gfx, thrLbl, _threads, rpcs3Lbl, _btnRpcs3 });
        UpdateRpcs3Button();
        rail.Controls.Add(opts); rail.Controls.Add(optsTitle); rail.Controls.Add(btnStack); rail.Controls.Add(railTitle); rail.Controls.Add(_logo);

        var right = new TableLayoutPanel { Dock = DockStyle.Fill, ColumnCount = 1, RowCount = 3, BackColor = Bg, Padding = new Padding(8, 0, 0, 0) };
        right.RowStyles.Add(new RowStyle(SizeType.Absolute, 40));
        right.RowStyles.Add(new RowStyle(SizeType.Percent, 100f));
        right.RowStyles.Add(new RowStyle(SizeType.Absolute, 36));
        _project.Text = "No project loaded"; _project.ForeColor = TextSec; _project.BackColor = Panel;
        _project.Dock = DockStyle.Fill; _project.TextAlign = ContentAlignment.MiddleLeft; _project.Padding = new Padding(12, 0, 0, 0);
        StyleTextBox(_log); StyleTextBox(_report); StyleTextBox(_roadmap);
        _roadmap.Text = "PPSX33 · Sequential: Load ELF → Decompile → Build → Run\r\n";
        _tabs.Dock = DockStyle.Fill;
        AddTab("Console", _log); AddTab("Report", _report); AddTab("Roadmap", _roadmap);
        _status.Text = "Ready"; _status.ForeColor = TextSec; _status.Dock = DockStyle.Fill;
        _status.TextAlign = ContentAlignment.MiddleLeft; _status.Padding = new Padding(8, 0, 0, 0);
        _progress.Style = ProgressBarStyle.Continuous; _progress.Minimum = 0; _progress.Maximum = 100; _progress.Value = 0;
        _progress.Dock = DockStyle.Right; _progress.Width = 200; _progress.Visible = false;
        _eta.Text = ""; _eta.ForeColor = TextSec; _eta.Dock = DockStyle.Right; _eta.Width = 150; _eta.TextAlign = ContentAlignment.MiddleRight;
        var statusBar = new Panel { Dock = DockStyle.Fill, BackColor = Panel };
        statusBar.Controls.Add(_status); statusBar.Controls.Add(_eta); statusBar.Controls.Add(_progress);
        right.Controls.Add(_project, 0, 0); right.Controls.Add(_tabs, 0, 1); right.Controls.Add(statusBar, 0, 2);
        root.Controls.Add(rail, 0, 0); root.Controls.Add(right, 1, 0); Controls.Add(root);

        _btnDecrypt.Click += async (_, _) => await DecryptEbootAndLoad();
        _btnLoad.Click += (_, _) => LoadElf(null);
        _btnLift.Click += async (_, _) => await RunStep("Decompiling…", _ => {
            bool ok = Native.Lift(_projectDir!, out var l);
            BeginInvoke(() => {
                _report.Text = l; Append(l);
                var m = System.Text.RegularExpressions.Regex.Match(l, @"chunks:\s*(\d+)", System.Text.RegularExpressions.RegexOptions.IgnoreCase);
                if (m.Success && int.TryParse(m.Groups[1].Value, out int n)) _liftChunks = n;
            });
            return ok;
        }, afterLift: true);
        _btnBuild.Click += async (_, _) => await BuildWithProgress();
        _btnRun.Click += async (_, _) => await RunGameExe();
        _btnCopy.Click += (_, _) => CopyToGameFolder();
        _btnCancel.Click += (_, _) => { _cts?.Cancel(); StopGame(); SetStatus("Stopped"); };
        Append("PPSX33 ready · core v" + SafeVersion());
        Append("Settings: " + Settings.DataDir);
        UpdateButtons();
    }

    protected override void OnHandleCreated(EventArgs e) {
        base.OnHandleCreated(e);
        try { int dark = 1; DwmSetWindowAttribute(Handle, 20, ref dark, sizeof(int)); } catch { }
    }

    static Label MakeLbl(string t, Color c, bool bold) => new() { Text = t, ForeColor = c, BackColor = Color.Transparent, Font = new Font("Segoe UI", bold ? 8f : 9f, bold ? FontStyle.Bold : FontStyle.Regular), AutoSize = true };
    void StyleBtn(Button b, string text, Color bg) { b.Text = text; b.FlatStyle = FlatStyle.Flat; b.FlatAppearance.BorderSize = 0; b.BackColor = bg; b.ForeColor = TextPri; b.Cursor = Cursors.Hand; b.UseVisualStyleBackColor = false; }
    void StyleTextBox(TextBox t) { t.Multiline = true; t.ScrollBars = ScrollBars.Both; t.WordWrap = true; t.ReadOnly = true; t.BackColor = Color.FromArgb(14, 14, 18); t.ForeColor = Color.FromArgb(200, 210, 220); t.BorderStyle = BorderStyle.None; t.Font = new Font("Consolas", 9f); t.Dock = DockStyle.Fill; }
    void AddTab(string title, Control body) { var page = new TabPage(title) { BackColor = Panel, Padding = new Padding(4) }; page.Controls.Add(body); _tabs.TabPages.Add(page); }
    void Append(string s) { if (string.IsNullOrEmpty(s)) return; if (_log.InvokeRequired) { BeginInvoke(() => Append(s)); return; } _log.AppendText(s.TrimEnd() + Environment.NewLine); }

    void SetStatus(string s, bool busy = false, int? percent = null, string? eta = null) {
        if (InvokeRequired) { BeginInvoke(() => SetStatus(s, busy, percent, eta)); return; }
        _status.Text = s;
        _status.ForeColor = s.Contains("fail", StringComparison.OrdinalIgnoreCase) || s.Contains("Error", StringComparison.OrdinalIgnoreCase) || s.Contains("stall", StringComparison.OrdinalIgnoreCase) ? Danger : (busy ? Accent : TextSec);
        _progress.Visible = busy || percent.HasValue;
        if (percent.HasValue) { _progress.Style = ProgressBarStyle.Continuous; _progress.MarqueeAnimationSpeed = 0; _progress.Value = Math.Clamp(percent.Value, 0, 100); }
        else if (busy) { _progress.Style = ProgressBarStyle.Marquee; _progress.MarqueeAnimationSpeed = 30; }
        else { _progress.Style = ProgressBarStyle.Continuous; _progress.MarqueeAnimationSpeed = 0; _progress.Value = 0; }
        _eta.Text = eta ?? "";
    }

    void UpdateRpcs3Button() {
        if (_btnRpcs3 == null) return;
        bool need = string.IsNullOrWhiteSpace(_settings.Rpcs3Path) || !File.Exists(_settings.Rpcs3Path);
        _btnRpcs3.Visible = need;
        if (_btnRpcs3.Parent != null)
            foreach (Control c in _btnRpcs3.Parent.Controls)
                if (c is Label lb && lb.Text.StartsWith("RPCS3", StringComparison.Ordinal)) lb.Visible = need;
    }

    void UpdateButtons() {
        _btnLift.Enabled = _canLift && !_busy; _btnBuild.Enabled = _canBuild && !_busy;
        _btnRun.Enabled = _canRun && !_busy; _btnCopy.Enabled = _canCopy && !_busy;
        _btnCancel.Enabled = _busy || _gameProc != null; _btnDecrypt.Enabled = !_busy; _btnLoad.Enabled = !_busy;
        _gfx.Enabled = _gfxUnlocked && !_busy;
    }

    async Task RunStep(string status, Func<CancellationToken, bool> work, bool afterLift = false, bool afterBuild = false) {
        if (_projectDir == null) { Append("No project."); return; }
        _cts?.Cancel(); _cts = new CancellationTokenSource();
        _busy = true; UpdateButtons(); SetStatus(status, true); SaveSettings();
        try {
            bool ok = await Task.Run(() => work(_cts.Token), _cts.Token);
            if (afterLift && ok) { _canBuild = true; _gfxUnlocked = true; }
            if (afterBuild && ok) { _canCopy = true; _canRun = true; }
            SetStatus(ok ? "Done" : "Failed");
            if (ok && afterLift) Append("Decompile OK — Build and Graphics backend unlocked.");
            if (ok && afterBuild) Append("Build OK — Run and Copy unlocked.");
        } catch (OperationCanceledException) { SetStatus("Cancelled"); }
        catch (Exception ex) { Append(ex.Message); SetStatus("Error"); }
        finally { _busy = false; UpdateButtons(); }
    }

    void LoadElf(string? path) {
        if (path == null) {
            using var ofd = new OpenFileDialog {
                Title = "Select decrypted ELF or SELF",
                Filter = "ELF / SELF|*.elf;*.ELF;*.self;*.SELF|All files|*.*",
                CheckFileExists = true,
                InitialDirectory = !string.IsNullOrWhiteSpace(_settings.LastElfPath) && File.Exists(_settings.LastElfPath) ? (Path.GetDirectoryName(_settings.LastElfPath) ?? "") : ""
            };
            if (ofd.ShowDialog(this) != DialogResult.OK) return;
            path = ofd.FileName; _settings.LastElfPath = path; SaveSettings();
        }
        string ext = Path.GetExtension(path).ToLowerInvariant();
        if (ext is not (".elf" or ".self")) { Append("Only .elf / .self. Use Decrypt for EBOOT.BIN."); SetStatus("Invalid file type"); return; }
        using var dlg = new PromptDialog("Project name", SuggestProjectName(path));
        if (dlg.ShowDialog(this) != DialogResult.OK) return;
        Directory.CreateDirectory(ProjectsRoot);
        if (!Native.CreateProject(ProjectsRoot, dlg.Value, path, out var proj, out var log)) { Append(log); SetStatus("Project create failed"); return; }
        _projectDir = proj; _project.Text = proj; _canLift = true; _canBuild = _canCopy = _canRun = false; _gfxUnlocked = false;
        Append(log); Append("ELF loaded — Decompile is now available."); SetStatus("Project ready — Decompile next"); UpdateButtons();
    }

    static string SuggestProjectName(string path) {
        try {
            string name = Path.GetFileNameWithoutExtension(path);
            if (string.IsNullOrWhiteSpace(name)) name = Path.GetFileName(path);
            if (string.IsNullOrWhiteSpace(name)) return "project";
            foreach (char c in Path.GetInvalidFileNameChars()) name = name.Replace(c, '_');
            name = name.Replace(' ', '_');
            if (name.Length > 64) name = name.Substring(0, 64);
            return name;
        } catch { } return "project";
    }

    async Task RunGameExe() {
        if (_projectDir == null) return;
        string exe = Path.Combine(_projectDir, "output", "game.exe");
        if (!File.Exists(exe)) { Append("game.exe not found — Build first."); return; }
        StopGame(); SaveSettings();
        var psi = new ProcessStartInfo { FileName = exe, WorkingDirectory = Path.GetDirectoryName(exe)!, UseShellExecute = false, RedirectStandardOutput = true, RedirectStandardError = true, CreateNoWindow = true };
        psi.Environment["PS3RT_GFX"] = _gfx.SelectedItem?.ToString() ?? "D3D11";
        _gameProc = new Process { StartInfo = psi, EnableRaisingEvents = true };
        var lastOut = DateTime.UtcNow;
        void OnData(string? line) { if (line == null) return; lastOut = DateTime.UtcNow; BeginInvoke(() => Append(line)); }
        _gameProc.OutputDataReceived += (_, e) => OnData(e.Data);
        _gameProc.ErrorDataReceived += (_, e) => OnData(e.Data);
        try {
            _gameProc.Start(); _gameProc.BeginOutputReadLine(); _gameProc.BeginErrorReadLine();
            _busy = true; UpdateButtons(); SetStatus("Running game.exe…", true);
            var start = DateTime.UtcNow;
            while (!_gameProc.HasExited) {
                await Task.Delay(500);
                if ((DateTime.UtcNow - lastOut).TotalMilliseconds > StallQuietMs && (DateTime.UtcNow - start).TotalMilliseconds > 8000) {
                    Append("[ui] no output for " + StallQuietMs + "ms — possible stall"); lastOut = DateTime.UtcNow;
                }
                if ((DateTime.UtcNow - start).TotalMilliseconds > MaxRunMs) { Append("[ui] max run time — stopping"); StopGame(); break; }
            }
            Append("game.exe exited code=" + (_gameProc?.ExitCode ?? -1)); SetStatus("Run finished");
        } catch (Exception ex) { Append(ex.Message); SetStatus("Run error"); }
        finally { _busy = false; UpdateButtons(); _gameProc = null; }
    }

    void StopGame() { try { if (_gameProc is { HasExited: false }) _gameProc.Kill(true); } catch { } _gameProc = null; }

    void CopyToGameFolder() {
        if (_projectDir == null) return;
        try {
            string src = Path.Combine(_projectDir, "output", "game.exe");
            if (!File.Exists(src)) { Append("game.exe missing"); return; }
            string? destDir = !string.IsNullOrWhiteSpace(_settings.LastElfPath) ? Path.GetDirectoryName(_settings.LastElfPath) : null;
            if (string.IsNullOrWhiteSpace(destDir) || !Directory.Exists(destDir)) {
                using var fbd = new FolderBrowserDialog { Description = "Select game folder" };
                if (fbd.ShowDialog(this) != DialogResult.OK) return;
                destDir = fbd.SelectedPath;
            }
            string dest = Path.Combine(destDir, "game.exe"); File.Copy(src, dest, true);
            Append("Copied to " + dest); SetStatus("Copied");
        } catch (Exception ex) { Append(ex.Message); SetStatus("Copy failed"); }
    }

    async Task BuildWithProgress() {
        if (_projectDir == null) { Append("No project."); return; }
        _cts?.Cancel(); _cts = new CancellationTokenSource();
        _busy = true; UpdateButtons(); SaveSettings();
        int chunks = Math.Max(_liftChunks, 1);
        int totalUnits = chunks + 8;
        string objDir = Path.Combine(_projectDir, "output", "obj");
        var sw = System.Diagnostics.Stopwatch.StartNew();
        int lastDone = 0; double? secPerUnit = null;
        SetStatus($"Building… 0/{totalUnits}", true, 0, "ETA —");
        var buildTask = Task.Run(() => { bool ok = Native.Build(_projectDir!, RuntimeDir, CompilersDir, out var l); return (ok, l); }, _cts.Token);
        try {
            while (!buildTask.IsCompleted) {
                await Task.Delay(350, _cts.Token);
                int done = CountBuildUnits(objDir, chunks);
                if (done > lastDone) { lastDone = done; if (done > 0 && sw.Elapsed.TotalSeconds > 0.5) secPerUnit = sw.Elapsed.TotalSeconds / done; }
                int pct = (int)Math.Clamp(done * 100.0 / totalUnits, 0, 95);
                if (done == 0) {
                    double est = 12 + chunks * 0.4; double remain = Math.Max(0, est - sw.Elapsed.TotalSeconds);
                    string eta0 = remain > 60 ? $"ETA ~{remain / 60:0.0}m" : $"ETA ~{remain:0}s";
                    SetStatus("Building… compiling runtime…", true, Math.Min(pct, 6), eta0);
                } else {
                    double remain = secPerUnit.HasValue ? Math.Max(0, (totalUnits - done) * secPerUnit.Value) : Math.Max(0, (totalUnits - done) * 0.4);
                    string eta = remain > 60 ? $"ETA ~{remain / 60:0.0}m" : $"ETA ~{remain:0}s";
                    SetStatus($"Building… {done}/{totalUnits} ({pct}%)", true, pct, eta);
                }
            }
            var (ok, log) = await buildTask; Append(log);
            if (ok) { _canCopy = true; _canRun = true; Append("Build OK — Run and Copy unlocked."); }
            SetStatus(ok ? "Done" : "Failed", false, ok ? 100 : 0, "");
        } catch (OperationCanceledException) { SetStatus("Cancelled"); }
        catch (Exception ex) { Append(ex.Message); SetStatus("Error"); }
        finally { _busy = false; UpdateButtons(); }
    }

    static int CountBuildUnits(string objDir, int expectedChunks) {
        int n = 0;
        try {
            if (!Directory.Exists(objDir)) return 0;
            foreach (var name in new[] { "ps3rt.obj", "spu_stub.obj", "rsx_stub.obj", "game_main.obj", "ppu_chunks.obj" })
                if (File.Exists(Path.Combine(objDir, name))) n++;
            int chunkObjs = 0;
            foreach (var f in Directory.EnumerateFiles(objDir, "ppu_chunk_*.obj")) { chunkObjs++; if (chunkObjs >= expectedChunks) break; }
            n += chunkObjs;
            string exe = Path.Combine(Path.GetDirectoryName(objDir) ?? "", "game.exe");
            if (File.Exists(exe)) n += 3;
        } catch { }
        return n;
    }

    void SaveSettings() {
        _settings.GraphicsBackend = _gfx.SelectedItem?.ToString() ?? "D3D11";
        _settings.PpuThreads = (int)_threads.Value;
        _settings.Save();
    }

    static string SafeVersion() {
        try { return Native.Version().ToString(); } catch { return "1"; }
    }
}
