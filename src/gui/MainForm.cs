using System.Diagnostics;
using System.Runtime.InteropServices;
using System.Text;

namespace PS3Recomp.Gui;

/// <summary>Static dark WinForms shell (no animations). C# UI + C++ ps3core/ps3rt.</summary>
public sealed class MainForm : Form
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
    private string? _projectDir;
    private CancellationTokenSource? _cts;
    private Process? _gameProc;
    private bool _canBuild, _canCopy, _canRun, _busy;

    private const int StallQuietMs = 4000;
    private const int MaxRunMs = 120_000;
    private static string RuntimeDir => Path.Combine(AppContext.BaseDirectory, "runtime");
    private static string CompilersDir => Path.Combine(AppContext.BaseDirectory, "Compilers-files");

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

        var root = new TableLayoutPanel { Dock = DockStyle.Fill, ColumnCount = 2, RowCount = 1, Padding = new Padding(10), BackColor = Bg };
        root.ColumnStyles.Add(new ColumnStyle(SizeType.Absolute, 250));
        root.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 100f));

        var rail = new Panel { Dock = DockStyle.Fill, BackColor = Panel, Padding = new Padding(12) };
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
        var opts = new Panel { Dock = DockStyle.Top, Height = 110, BackColor = Color.Transparent };
        var gfxLbl = MakeLbl("Graphics backend", TextSec, false); gfxLbl.Location = new Point(0, 4);
        _gfx.DropDownStyle = ComboBoxStyle.DropDownList; _gfx.FlatStyle = FlatStyle.Flat;
        _gfx.BackColor = Panel2; _gfx.ForeColor = TextPri; _gfx.Location = new Point(0, 24); _gfx.Size = new Size(214, 28);
        _gfx.Items.AddRange(new object[] { "D3D10", "D3D11", "Vulkan" });
        _gfx.SelectedItem = _settings.GraphicsBackend; if (_gfx.SelectedIndex < 0) _gfx.SelectedIndex = 1;
        var thrLbl = MakeLbl("CPU threads", TextSec, false); thrLbl.Location = new Point(0, 58);
        _threads.Minimum = 1; _threads.Maximum = 64; _threads.BackColor = Panel2; _threads.ForeColor = TextPri;
        _threads.Location = new Point(0, 78); _threads.Size = new Size(100, 26);
        _threads.Value = Math.Clamp(_settings.PpuThreads, 1, 64);
        opts.Controls.AddRange(new Control[] { gfxLbl, _gfx, thrLbl, _threads });
        rail.Controls.Add(opts); rail.Controls.Add(optsTitle); rail.Controls.Add(btnStack); rail.Controls.Add(railTitle);

        var right = new TableLayoutPanel { Dock = DockStyle.Fill, ColumnCount = 1, RowCount = 3, BackColor = Bg, Padding = new Padding(8, 0, 0, 0) };
        right.RowStyles.Add(new RowStyle(SizeType.Absolute, 40));
        right.RowStyles.Add(new RowStyle(SizeType.Percent, 100f));
        right.RowStyles.Add(new RowStyle(SizeType.Absolute, 36));
        _project.Text = "No project loaded"; _project.ForeColor = TextSec; _project.BackColor = Panel;
        _project.Dock = DockStyle.Fill; _project.TextAlign = ContentAlignment.MiddleLeft; _project.Padding = new Padding(12, 0, 0, 0);
        StyleTextBox(_log); StyleTextBox(_report); StyleTextBox(_roadmap);
        _roadmap.Text = "PPSX33: static UI · NID table · RSX host present (D3D10/11/Vulkan)\r\n";
        _tabs.Dock = DockStyle.Fill;
        AddTab("Console", _log); AddTab("Report", _report); AddTab("Roadmap", _roadmap);
        _status.Text = "Ready"; _status.ForeColor = TextSec; _status.Dock = DockStyle.Fill;
        _status.TextAlign = ContentAlignment.MiddleLeft; _status.Padding = new Padding(8, 0, 0, 0);
        _progress.Style = ProgressBarStyle.Continuous; _progress.Dock = DockStyle.Right; _progress.Width = 180; _progress.Visible = false;
        var statusBar = new Panel { Dock = DockStyle.Fill, BackColor = Panel };
        statusBar.Controls.Add(_status); statusBar.Controls.Add(_progress);
        right.Controls.Add(_project, 0, 0); right.Controls.Add(_tabs, 0, 1); right.Controls.Add(statusBar, 0, 2);
        root.Controls.Add(rail, 0, 0); root.Controls.Add(right, 1, 0); Controls.Add(root);

        _btnDecrypt.Click += async (_, _) => await DecryptEbootAndLoad();
        _btnLoad.Click += (_, _) => LoadElf(null);
        _btnLift.Click += async (_, _) => await RunStep("Decompiling…", _ => { bool ok = Native.Lift(_projectDir!, out var l); BeginInvoke(() => { _report.Text = l; Append(l); }); return ok; }, enableBuild: true);
        _btnBuild.Click += async (_, _) => await RunStep("Building…", _ => { bool ok = Native.Build(_projectDir!, RuntimeDir, CompilersDir, out var l); BeginInvoke(() => Append(l)); return ok; }, enableCopy: true, enableRun: true);
        _btnRun.Click += async (_, _) => await RunGameExe();
        _btnCopy.Click += (_, _) => CopyToEbootFolder();
        _btnCancel.Click += (_, _) => StopAll();
        Append($"ps3core {SafeVersion()} · static UI · NID + RSX host present");
    }

    protected override void OnHandleCreated(EventArgs e)
    {
        base.OnHandleCreated(e);
        try { int dark = 1; DwmSetWindowAttribute(Handle, 20, ref dark, sizeof(int)); } catch { }
    }

    static Label MakeLbl(string t, Color c, bool bold) => new() { Text = t, ForeColor = c, BackColor = Color.Transparent, Font = new Font("Segoe UI", bold ? 8f : 9f, bold ? FontStyle.Bold : FontStyle.Regular), AutoSize = true };
    void StyleBtn(Button b, string text, Color bg) { b.Text = text; b.FlatStyle = FlatStyle.Flat; b.FlatAppearance.BorderSize = 0; b.BackColor = bg; b.ForeColor = TextPri; b.Cursor = Cursors.Hand; b.UseVisualStyleBackColor = false; }
    void StyleTextBox(TextBox t) { t.Multiline = true; t.ScrollBars = ScrollBars.Both; t.WordWrap = false; t.ReadOnly = true; t.BackColor = Color.FromArgb(14, 14, 18); t.ForeColor = Color.FromArgb(200, 210, 220); t.BorderStyle = BorderStyle.None; t.Font = new Font("Consolas", 9f); t.Dock = DockStyle.Fill; }
    void AddTab(string title, Control body) { var page = new TabPage(title) { BackColor = Panel, Padding = new Padding(4) }; page.Controls.Add(body); _tabs.TabPages.Add(page); }
    void Append(string s) { if (string.IsNullOrEmpty(s)) return; if (_log.InvokeRequired) { BeginInvoke(() => Append(s)); return; } _log.AppendText(s.TrimEnd() + Environment.NewLine); }
    void SetStatus(string s, bool busy = false)
    {
        if (InvokeRequired) { BeginInvoke(() => SetStatus(s, busy)); return; }
        _status.Text = s;
        _status.ForeColor = s.Contains("fail", StringComparison.OrdinalIgnoreCase) || s.Contains("Error", StringComparison.OrdinalIgnoreCase) || s.Contains("stall", StringComparison.OrdinalIgnoreCase) ? Danger : (busy ? Accent : TextSec);
        _progress.Visible = busy;
        if (busy) { _progress.Style = ProgressBarStyle.Marquee; _progress.MarqueeAnimationSpeed = 30; }
        else { _progress.Style = ProgressBarStyle.Continuous; _progress.MarqueeAnimationSpeed = 0; _progress.Value = 0; }
    }
    void UpdateButtons()
    {
        _btnLift.Enabled = _projectDir != null && !_busy;
        _btnBuild.Enabled = _canBuild && !_busy;
        _btnRun.Enabled = _canRun && !_busy;
        _btnCopy.Enabled = _canCopy && !_busy;
        _btnCancel.Enabled = _busy || _gameProc != null;
        _btnDecrypt.Enabled = !_busy; _btnLoad.Enabled = !_busy;
    }

    async Task RunStep(string status, Func<CancellationToken, bool> work, bool enableBuild = false, bool enableCopy = false, bool enableRun = false)
    {
        if (_projectDir == null) { Append("No project."); return; }
        _cts?.Cancel(); _cts = new CancellationTokenSource();
        _busy = true; UpdateButtons(); SetStatus(status, true); SaveSettings();
        try
        {
            bool ok = await Task.Run(() => work(_cts.Token), _cts.Token);
            if (enableBuild) _canBuild = ok; if (enableCopy) _canCopy = ok; if (enableRun) _canRun = ok;
            SetStatus(ok ? "Done" : "Failed");
        }
        catch (OperationCanceledException) { SetStatus("Cancelled"); }
        catch (Exception ex) { Append(ex.Message); SetStatus("Error"); }
        finally { _busy = false; UpdateButtons(); }
    }

    void LoadElf(string? path)
    {
        if (path == null)
        {
            using var ofd = new OpenFileDialog { Filter = "ELF|*.elf;*.ELF;*.bin|All|*.*" };
            if (ofd.ShowDialog(this) != DialogResult.OK) return;
            path = ofd.FileName;
        }
        using var dlg = new PromptDialog("Project name", "GOW3");
        if (dlg.ShowDialog(this) != DialogResult.OK) return;
        string root = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.MyDocuments), "PPSX33");
        Directory.CreateDirectory(root);
        if (!Native.CreateProject(root, dlg.Value, path, out var proj, out var log))
        { Append(log); SetStatus("Project create failed"); return; }
        _projectDir = proj; _project.Text = proj; _canBuild = _canCopy = _canRun = false;
        Append(log); SetStatus("Project ready"); UpdateButtons();
    }

    async Task DecryptEbootAndLoad()
    {
        using var ofd = new OpenFileDialog { Filter = "EBOOT|EBOOT.BIN;*.BIN|All|*.*" };
        if (ofd.ShowDialog(this) != DialogResult.OK) return;
        SetStatus("Decrypting…", true); _busy = true; UpdateButtons();
        try
        {
            string elf = "", err = "";
            bool ok = await Task.Run(() => Rpcs3Launcher.DecryptBinary(_settings.Rpcs3Path ?? "", ofd.FileName, out elf, out err));
            if (ok && File.Exists(elf)) { Append("Decrypted: " + elf); LoadElf(elf); }
            else { if (!string.IsNullOrEmpty(err)) Append(err); SetStatus("Decrypt failed"); }
        }
        catch (Exception ex) { Append(ex.Message); SetStatus("Decrypt error"); }
        finally { _busy = false; UpdateButtons(); }
    }

    async Task RunGameExe()
    {
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
        try
        {
            _gameProc.Start(); _gameProc.BeginOutputReadLine(); _gameProc.BeginErrorReadLine();
            _busy = true; UpdateButtons(); SetStatus("Running game.exe…", true);
            var start = DateTime.UtcNow;
            while (!_gameProc.HasExited)
            {
                await Task.Delay(200);
                if ((DateTime.UtcNow - lastOut).TotalMilliseconds > StallQuietMs) { Append("[ui] stall — stopping"); try { _gameProc.Kill(true); } catch { } SetStatus("Stopped (stall)"); break; }
                if ((DateTime.UtcNow - start).TotalMilliseconds > MaxRunMs) { Append("[ui] timeout — stopping"); try { _gameProc.Kill(true); } catch { } SetStatus("Stopped (timeout)"); break; }
            }
            if (_gameProc.HasExited && _status.Text.StartsWith("Running")) SetStatus($"Exit code {_gameProc.ExitCode}");
        }
        catch (Exception ex) { Append(ex.Message); SetStatus("Run failed"); }
        finally { _gameProc?.Dispose(); _gameProc = null; _busy = false; UpdateButtons(); }
    }

    void StopGame() { try { if (_gameProc is { HasExited: false }) _gameProc.Kill(true); } catch { } _gameProc = null; }
    void StopAll() { _cts?.Cancel(); StopGame(); SetStatus("Stopped"); _busy = false; UpdateButtons(); }
    void CopyToEbootFolder()
    {
        if (_projectDir == null) return;
        string outDir = Path.Combine(_projectDir, "output");
        string? destParent = Directory.GetParent(_projectDir)?.FullName;
        if (destParent == null) return;
        foreach (var name in new[] { "game.exe", "ps3rt.dll", "guest_image.bin" })
        {
            string src = Path.Combine(outDir, name);
            if (File.Exists(src)) { File.Copy(src, Path.Combine(destParent, name), true); Append("Copied " + name); }
        }
        SetStatus("Copied");
    }
    void SaveSettings() { _settings.GraphicsBackend = _gfx.SelectedItem?.ToString() ?? "D3D11"; _settings.PpuThreads = (int)_threads.Value; _settings.Save(); }
    static string SafeVersion() { try { return Native.Version().ToString(); } catch { return "?"; } }
}
