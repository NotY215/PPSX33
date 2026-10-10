using System.Diagnostics;
using System.Text;

using PS3Recomp.Gui.Ui;

namespace PS3Recomp.Gui;

/// <summary>
/// PPSX33 main window. PS3-themed glass / liquid-glass spatial UI (visuals live in Ui/*.cs).
/// Workflow: Decrypt -> Load -> Decompile -> Build -> Run -> Copy.
/// </summary>
public sealed class MainForm : Form
{
    static readonly Color Bg = Theme.Void;
    static readonly Color Panel2 = Color.FromArgb(16, 22, 46);
    static readonly Color TextPri = Theme.TextPri;
    static readonly Color TextSec = Theme.TextSec;
    static readonly Color Danger = Theme.Danger;

    private readonly Settings _settings = Settings.Load();
    private readonly TextBox _log = new();
    private readonly TextBox _report = new();
    private readonly Label _project = new();
    private readonly Label _status = new();
    private readonly GlassProgress _progress = new();
    private readonly GlassButton _btnDecrypt = new();
    private readonly GlassButton _btnLoad = new();
    private readonly GlassButton _btnLift = new();
    private readonly GlassButton _btnBuild = new();
    private readonly GlassButton _btnRun = new();
    private readonly GlassButton _btnCopy = new();
    private readonly GlassButton _btnCancel = new();
    private readonly ComboBox _gfx = new();
    private readonly NumericUpDown _threads = new();
    private string? _projectDir;
    private CancellationTokenSource? _cts;
    private Process? _gameProc;
    private bool _canBuild;
    private bool _canCopy;
    private bool _canRun;

    // Stall detection: no new stdout/stderr for this many ms -> kill
    private const int StallQuietMs = 4000;
    private const int MaxRunMs = 120_000;

    private static string RuntimeDir => Path.Combine(AppContext.BaseDirectory, "runtime");
    private static string CompilersDir => Path.Combine(AppContext.BaseDirectory, "Compilers-files");

    // Layered-window compositing = flicker-free animated glass.
    protected override CreateParams CreateParams
    {
        get { var cp = base.CreateParams; cp.ExStyle |= 0x02000000; return cp; }
    }

    protected override void OnHandleCreated(EventArgs e)
    {
        base.OnHandleCreated(e);
        UiNative.DarkTitleBar(Handle);
    }

    public MainForm()
    {
        AutoScaleMode = AutoScaleMode.Dpi;
        AutoScaleDimensions = new SizeF(96f, 96f);
        Text = "PPSX33  ·  PlayStation 3 Recompiler";
        Width = 1180;
        Height = 780;
        MinimumSize = new Size(1020, 740);
        StartPosition = FormStartPosition.CenterScreen;
        BackColor = Bg;
        ForeColor = TextPri;
        Font = Theme.Font(9.25f);
        DoubleBuffered = true;

        // ---- stage: animated XMB-style backdrop hosting every glass panel
        var stage = new LiquidBackdrop { Dock = DockStyle.Fill, Padding = new Padding(8) };
        var grid = new FlowGrid { Dock = DockStyle.Fill, ColumnCount = 2, RowCount = 3 };
        grid.ColumnStyles.Add(new ColumnStyle(SizeType.Absolute, 280));
        grid.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 100));
        grid.RowStyles.Add(new RowStyle(SizeType.Absolute, 80));
        grid.RowStyles.Add(new RowStyle(SizeType.Percent, 100));
        grid.RowStyles.Add(new RowStyle(SizeType.Absolute, 50));

        // ---- header
        var header = new HeaderBar { Dock = DockStyle.Fill, Radius = 20 };

        // ---- workflow rail
        var rail = new GlassPanel { Dock = DockStyle.Fill, AutoScroll = true };
        rail.SetInset(16, 12, 16, 12);
        var railTitle = new Label
        {
            Text = "WORKFLOW",
            ForeColor = Theme.TextSec,
            BackColor = Color.Transparent,
            Font = Theme.Semi(8f),
            Dock = DockStyle.Top,
            Height = 28,
            TextAlign = ContentAlignment.MiddleLeft
        };

        StylePrimaryButton(_btnDecrypt, "Decrypt EBOOT", 0);
        StyleStepButton(_btnLoad, "Load ELF", 1);
        StyleStepButton(_btnLift, "Decompile", 2);
        StyleStepButton(_btnBuild, "Build", 3);
        StylePrimaryButton(_btnRun, "Run game.exe", 4);
        StyleStepButton(_btnCopy, "Copy to game folder", 5);
        StyleDangerButton(_btnCancel, "Stop / Cancel", 6);
        _btnLift.Enabled = false;
        _btnBuild.Enabled = false;
        _btnRun.Enabled = false;
        _btnCopy.Enabled = false;
        _btnCancel.Enabled = false;

        var btnStack = new FlowLayoutPanel
        {
            Dock = DockStyle.Top,
            FlowDirection = FlowDirection.TopDown,
            WrapContents = false,
            AutoSize = true,
            BackColor = Color.Transparent,
            Padding = new Padding(0, 2, 0, 0)
        };
        foreach (var b in new[] { _btnDecrypt, _btnLoad, _btnLift, _btnBuild, _btnRun, _btnCopy, _btnCancel })
        {
            b.Width = 224;
            b.Margin = new Padding(0, 0, 0, 7);
            btnStack.Controls.Add(b);
        }

        var optsTitle = new Label
        {
            Text = "OPTIONS",
            ForeColor = Theme.TextSec,
            BackColor = Color.Transparent,
            Font = Theme.Semi(8f),
            Dock = DockStyle.Top,
            Height = 32,
            TextAlign = ContentAlignment.BottomLeft
        };
        var optsPanel = new Panel { Dock = DockStyle.Top, Height = 132, BackColor = Color.Transparent };

        var gfxLbl = new Label { Text = "Graphics backend", ForeColor = TextSec, BackColor = Color.Transparent, Location = new Point(0, 6), AutoSize = true };
        _gfx.DropDownStyle = ComboBoxStyle.DropDownList;
        _gfx.FlatStyle = FlatStyle.Flat;
        _gfx.BackColor = Panel2;
        _gfx.ForeColor = TextPri;
        _gfx.DrawMode = DrawMode.OwnerDrawFixed;
        _gfx.ItemHeight = 22;
        _gfx.DrawItem += (s, e) =>
        {
            if (e.Index < 0) return;
            bool sel = (e.State & DrawItemState.Selected) != 0 && (e.State & DrawItemState.ComboBoxEdit) == 0;
            using (var bg = new SolidBrush(sel ? Color.FromArgb(56, 92, 190) : Panel2)) e.Graphics.FillRectangle(bg, e.Bounds);
            TextRenderer.DrawText(e.Graphics, _gfx.Items[e.Index]?.ToString() ?? "", _gfx.Font,
                new Rectangle(e.Bounds.X + 2, e.Bounds.Y, e.Bounds.Width - 2, e.Bounds.Height), TextPri,
                TextFormatFlags.Left | TextFormatFlags.VerticalCenter | TextFormatFlags.NoPadding);
        };
        _gfx.Items.AddRange(new object[] { "D3D10", "D3D11", "Vulkan" });
        _gfx.SelectedItem = _settings.GraphicsBackend;
        if (_gfx.SelectedIndex < 0) _gfx.SelectedIndex = 1;
        UiNative.DarkControl(_gfx, "DarkMode_CFD");
        var gfxFrame = new FieldFrame { Fill = Panel2, Location = new Point(0, 28), Size = new Size(224, 34), Padding = new Padding(10, 0, 6, 0) };
        gfxFrame.Controls.Add(_gfx);

        var thrLbl = new Label { Text = "CPU threads", ForeColor = TextSec, BackColor = Color.Transparent, Location = new Point(0, 74), AutoSize = true };
        _threads.Minimum = 1;
        _threads.Maximum = 64;
        _threads.BorderStyle = BorderStyle.None;
        _threads.BackColor = Panel2;
        _threads.ForeColor = TextPri;
        _threads.Value = Math.Clamp(_settings.PpuThreads, 1, 64);
        UiNative.DarkControl(_threads, "DarkMode_Explorer");
        var thrFrame = new FieldFrame { Fill = Panel2, Location = new Point(0, 96), Size = new Size(104, 34), Padding = new Padding(10, 0, 6, 0) };
        thrFrame.Controls.Add(_threads);

        optsPanel.Controls.Add(gfxLbl);
        optsPanel.Controls.Add(gfxFrame);
        optsPanel.Controls.Add(thrLbl);
        optsPanel.Controls.Add(thrFrame);

        rail.Controls.Add(optsPanel);
        rail.Controls.Add(optsTitle);
        rail.Controls.Add(btnStack);
        rail.Controls.Add(railTitle);

        // ---- status bar
        var statusBar = new GlassPanel { Dock = DockStyle.Fill, Radius = 16 };
        statusBar.SetInset(18, 0, 14, 0);
        _status.Text = "Ready";
        _status.ForeColor = TextSec;
        _status.BackColor = Color.Transparent;
        _status.Font = Theme.Semi(9f);
        _status.AutoSize = false;
        _status.Dock = DockStyle.Fill;
        _status.TextAlign = ContentAlignment.MiddleLeft;
        _progress.Style = ProgressBarStyle.Marquee;
        _progress.MarqueeAnimationSpeed = 0;
        _progress.Width = 220;
        _progress.Dock = DockStyle.Right;
        statusBar.Controls.Add(_status);
        statusBar.Controls.Add(_progress);

        // ---- project strip
        var projStrip = new GlassPanel { Dock = DockStyle.Fill, Radius = 16 };
        projStrip.SetInset(44, 0, 14, 0);
        projStrip.Decorate = (g, r) =>
        {
            float k = Dpi.K(projStrip), cx = r.X + 24 * k, cy = r.Y + r.Height / 2f, s = 8 * k;
            using var pen = new Pen(Color.FromArgb(210, Theme.Cyan), 1.5f) { LineJoin = System.Drawing.Drawing2D.LineJoin.Round };
            g.DrawPolygon(pen, new[] { new PointF(cx, cy - s), new PointF(cx + s, cy - s * .5f), new PointF(cx + s, cy + s * .5f), new PointF(cx, cy + s), new PointF(cx - s, cy + s * .5f), new PointF(cx - s, cy - s * .5f) });
            using var db = new SolidBrush(Color.FromArgb(230, Theme.Cyan));
            g.FillEllipse(db, cx - 2 * k, cy - 2 * k, 4 * k, 4 * k);
        };
        _project.Text = "No project loaded";
        _project.ForeColor = TextSec;
        _project.BackColor = Color.Transparent;
        _project.AutoSize = false;
        _project.Dock = DockStyle.Fill;
        _project.TextAlign = ContentAlignment.MiddleLeft;
        projStrip.Controls.Add(_project);

        // ---- tabs (console / report / roadmap)
        var tabs = new GlassTabs { Dock = DockStyle.Fill };
        StyleTextBox(_log);
        StyleTextBox(_report);
        var roadmapBox = new TextBox();
        StyleTextBox(roadmapBox);
        roadmapBox.Text = RoadmapText;
        tabs.AddPage("Console", ConsoleFrame(_log));
        tabs.AddPage("Lift report", ConsoleFrame(_report));
        tabs.AddPage("Roadmap", ConsoleFrame(roadmapBox));

        var main = new FlowGrid { Dock = DockStyle.Fill, ColumnCount = 1, RowCount = 2 };
        main.RowStyles.Add(new RowStyle(SizeType.Absolute, 60));
        main.RowStyles.Add(new RowStyle(SizeType.Percent, 100));
        main.Controls.Add(projStrip, 0, 0);
        main.Controls.Add(tabs, 0, 1);

        grid.Controls.Add(header, 0, 0);
        grid.SetColumnSpan(header, 2);
        grid.Controls.Add(rail, 0, 1);
        grid.Controls.Add(main, 1, 1);
        grid.Controls.Add(statusBar, 0, 2);
        grid.SetColumnSpan(statusBar, 2);
        stage.Controls.Add(grid);
        Controls.Add(stage);

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
        }, enableCopy: true, enableRun: true);
        _btnRun.Click += async (_, _) => await RunGameExe();
        _btnCopy.Click += (_, _) => CopyToEbootFolder();
        _btnCancel.Click += (_, _) => StopAll();
        FormClosing += (_, e) =>
        {
            StopAll();
            SaveSettings();
        };

        Append($"ps3core {SafeVersion()}  ·  runtime {RuntimeDir}");
        Append("Decrypt: rpcs3.exe --decrypt \"EBOOT.BIN\"");
        Append("Run game.exe streams console output here; auto-stops after " + (StallQuietMs / 1000) + "s with no new output.");
    }

    private static FieldFrame ConsoleFrame(TextBox t)
    {
        var f = new FieldFrame { Fill = Theme.Console, Dock = DockStyle.Fill, FillChild = true, Padding = new Padding(14, 12, 6, 12), Radius = 14 };
        f.Controls.Add(t);
        return f;
    }

    private static void StyleTextBox(TextBox t)
    {
        t.Multiline = true;
        t.ReadOnly = true;
        t.ScrollBars = ScrollBars.Both;
        t.Dock = DockStyle.Fill;
        t.BorderStyle = BorderStyle.None;
        t.BackColor = Theme.Console;
        t.ForeColor = Color.FromArgb(196, 222, 255);
        t.Font = Theme.Mono(9.25f);
        t.WordWrap = false;
        UiNative.DarkControl(t, "DarkMode_Explorer");
    }

    private static void StylePrimaryButton(GlassButton b, string text, int glyph = -1)
    {
        b.Text = text;
        b.Kind = GlassKind.Primary;
        b.Glyph = glyph;
        b.Height = 40;
    }

    private static void StyleStepButton(GlassButton b, string text, int glyph = -1)
    {
        b.Text = text;
        b.Kind = GlassKind.Step;
        b.Glyph = glyph;
        b.Height = 38;
    }

    private static void StyleDangerButton(GlassButton b, string text, int glyph = -1)
    {
        b.Text = text;
        b.Kind = GlassKind.Danger;
        b.Glyph = glyph;
        b.Height = 36;
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
        _status.ForeColor = s.Contains("fail", StringComparison.OrdinalIgnoreCase) || s.Contains("Error") || s.Contains("stall", StringComparison.OrdinalIgnoreCase)
            ? Danger
            : TextSec;
    }

    private void SaveSettings()
    {
        _settings.GraphicsBackend = _gfx.SelectedItem?.ToString() ?? "D3D11";
        _settings.PpuThreads = (int)_threads.Value;
        _settings.Save();
    }

    private void StopAll()
    {
        try { _cts?.Cancel(); } catch { }
        try
        {
            if (_gameProc != null && !_gameProc.HasExited)
            {
                Append("[run] Stopping game.exe…");
                _gameProc.Kill(entireProcessTree: true);
            }
        }
        catch (Exception ex) { Append("[run] Stop error: " + ex.Message); }
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
        _canRun = File.Exists(Path.Combine(dir, "output", "game.exe"));
        _btnLift.Enabled = true;
        _btnBuild.Enabled = false;
        _btnRun.Enabled = _canRun;
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

    private async Task RunGameExe()
    {
        if (_projectDir == null) return;
        string exe = Path.Combine(_projectDir, "output", "game.exe");
        string work = Path.Combine(_projectDir, "output");
        if (!File.Exists(exe))
        {
            Append("ERROR: game.exe not found. Build first.");
            return;
        }

        Append("[run] Starting " + exe);
        Append("[run] Working dir: " + work);
        Append("[run] Auto-stop if no output for " + (StallQuietMs / 1000) + "s (max " + (MaxRunMs / 1000) + "s)");

        _cts?.Dispose();
        _cts = new CancellationTokenSource();
        var token = _cts.Token;
        SetBusy(true);
        SetStatus("Running game.exe…");
        _progress.MarqueeAnimationSpeed = 30;

        try
        {
            await Task.Run(() =>
            {
                var psi = new ProcessStartInfo
                {
                    FileName = exe,
                    WorkingDirectory = work,
                    UseShellExecute = false,
                    RedirectStandardOutput = true,
                    RedirectStandardError = true,
                    CreateNoWindow = true,
                    StandardOutputEncoding = Encoding.UTF8,
                    StandardErrorEncoding = Encoding.UTF8,
                };

                using var proc = new Process { StartInfo = psi, EnableRaisingEvents = true };
                _gameProc = proc;

                var lastOutput = DateTime.UtcNow;
                var started = DateTime.UtcNow;
                var lineLock = new object();

                void OnLine(string? line)
                {
                    if (line == null) return;
                    lock (lineLock)
                    {
                        lastOutput = DateTime.UtcNow;
                    }
                    Append(line);
                }

                proc.OutputDataReceived += (_, e) => OnLine(e.Data);
                proc.ErrorDataReceived += (_, e) => OnLine(e.Data);

                if (!proc.Start())
                    throw new InvalidOperationException("Failed to start game.exe");

                proc.BeginOutputReadLine();
                proc.BeginErrorReadLine();

                while (!proc.HasExited)
                {
                    if (token.IsCancellationRequested)
                    {
                        try { proc.Kill(entireProcessTree: true); } catch { }
                        Append("[run] Cancelled by user.");
                        break;
                    }

                    var now = DateTime.UtcNow;
                    double quiet;
                    lock (lineLock) quiet = (now - lastOutput).TotalMilliseconds;
                    double elapsed = (now - started).TotalMilliseconds;

                    if (quiet >= StallQuietMs)
                    {
                        Append("[run] No new output for " + (StallQuietMs / 1000) + "s — treating as stuck, stopping.");
                        try { proc.Kill(entireProcessTree: true); } catch { }
                        break;
                    }
                    if (elapsed >= MaxRunMs)
                    {
                        Append("[run] Max run time " + (MaxRunMs / 1000) + "s reached, stopping.");
                        try { proc.Kill(entireProcessTree: true); } catch { }
                        break;
                    }

                    Thread.Sleep(200);
                }

                try { proc.WaitForExit(2000); } catch { }
                int code = -1;
                try { code = proc.ExitCode; } catch { }
                Append("[run] Exit code: " + code);
                _gameProc = null;
            }, token);

            SetStatus("Run finished");
        }
        catch (OperationCanceledException)
        {
            Append("[run] CANCELLED");
            SetStatus("Cancelled");
        }
        catch (Exception ex)
        {
            Append("[run] ERROR: " + ex.Message);
            SetStatus("Run error");
        }
        finally
        {
            _gameProc = null;
            _progress.MarqueeAnimationSpeed = 0;
            SetBusy(false);
        }
    }

    private async Task RunStep(string title, Func<CancellationToken, (bool ok, string log)> work, bool enableBuild = false, bool enableCopy = false, bool enableRun = false, bool refreshReport = false)
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
            if (ok && enableRun) _canRun = true;
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
            Path.Combine(_projectDir, "output", "analysis_report.txt"),
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
        _btnRun.Enabled = !busy && _canRun;
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
        Append("Run game.exe from that folder or use Run game.exe in the UI.");
    }

    private const string RoadmapText =
        "PPSX33 Roadmap (summary)\r\n\r\n" +
        "Phase 1  UI              DONE  (dark UI, decrypt, Run game.exe)\r\n" +
        "Phase 2  PPU lift        GOW3 + Uncharted2 static 100%\r\n" +
        "Phase 3  PPU runtime     IN PROGRESS  (import stubs @ 0x39800000)\r\n" +
        "Phase 4  SPU             Interpreter\r\n" +
        "Phase 5  RSX             GCM/FIFO core\r\n" +
        "Phase 6  Native output   DONE\r\n\r\n" +
        "See ROADMAP.md and docs/games/GOW3/GOW3.md.\r\n";
}
