using System.Diagnostics;
using System.Text;

namespace PS3Recomp.Gui;

public sealed partial class MainForm
{
    void AddTab(string title, Control body) { var page = new TabPage(title) { BackColor = Panel, Padding = new Padding(4) }; page.Controls.Add(body); _tabs.TabPages.Add(page); }

    void StyleBtn(Control b, string text, Color bg)
    {
        b.Text = text;
        b.BackColor = bg;
        b.ForeColor = TextPri;
        b.Dock = DockStyle.Top;
        b.Height = 36;
        b.Margin = new Padding(0, 3, 0, 3);
        b.Padding = new Padding(12, 0, 12, 0);
        if (b is Button btn)
        {
            btn.FlatStyle = FlatStyle.Flat;
            btn.FlatAppearance.BorderSize = 1;
            btn.FlatAppearance.BorderColor = Color.FromArgb(0x33, 0x41, 0x55);
            btn.FlatAppearance.MouseOverBackColor = Color.FromArgb(0x02, 0x84, 0xC7);
            btn.TextAlign = ContentAlignment.MiddleLeft;
            btn.Cursor = Cursors.Hand;
            btn.Font = new Font("Segoe UI", 9f, FontStyle.Regular);
        }
    }
    void StyleTextBox(TextBox t) { t.Multiline = true; t.ScrollBars = ScrollBars.Both; t.Dock = DockStyle.Fill; t.BackColor = Color.FromArgb(7, 10, 15); t.ForeColor = TextPri; t.BorderStyle = BorderStyle.None; t.Font = new Font("Consolas", 9f); }
    void StyleCombo(ComboBox c) {
        c.FlatStyle = FlatStyle.Flat;
        c.BackColor = Panel2;
        c.ForeColor = TextPri;
        c.Font = new Font("Segoe UI", 9f);
        c.Dock = DockStyle.Top;
        c.Margin = new Padding(0, 0, 0, 8);
        c.DropDownStyle = ComboBoxStyle.DropDownList;
    }
    Label MakeLabel(string text) => MakeLbl(text, TextSec, true);
    Label MakeLbl(string text, Color c, bool bold) => new Label { Text = text, ForeColor = c, BackColor = Color.Transparent, Font = bold ? new Font("Segoe UI", 9f, FontStyle.Bold) : new Font("Segoe UI", 9f) };
    void DrawPs3Tab(object? sender, DrawItemEventArgs e) {
        if (e.Index < 0) return;
        var g = e.Graphics;
        var tab = _tabs.TabPages[e.Index];
        bool sel = (e.State & DrawItemState.Selected) != 0;
        using var br = new SolidBrush(sel ? Panel2 : Panel);
        g.FillRectangle(br, e.Bounds);
        if (sel)
        {
            using var pen = new Pen(Color.FromArgb(0x38, 0xBD, 0xF8), 2);
            g.DrawLine(pen, e.Bounds.Left + 8, e.Bounds.Bottom - 2, e.Bounds.Right - 8, e.Bounds.Bottom - 2);
        }
        TextRenderer.DrawText(g, tab.Text, Font, e.Bounds, sel ? Color.FromArgb(0x38, 0xBD, 0xF8) : TextSec, TextFormatFlags.HorizontalCenter | TextFormatFlags.VerticalCenter);
    }
    void Append(string s)
    {
        if (IsDisposed) return;
        if (InvokeRequired) { BeginInvoke(() => Append(s)); return; }
        _log.AppendText(s.TrimEnd() + Environment.NewLine);
    }

    void SetStatus(string s, bool busy = false, int? percent = null, string? eta = null)
    {
        if (InvokeRequired)
        {
            BeginInvoke(() => SetStatus(s, busy, percent, eta));
            return;
        }

        _status.Text = string.IsNullOrEmpty(eta) ? s : (s + " - " + eta);

        if (_headerStatus != null)
        {
            string header = busy ? "WORKING" : s;
            if (header.Length > 28)
                header = header.Substring(0, 28);
            _headerStatus.Text = header;
        }

        if (percent.HasValue)
        {
            _progress.Style = ProgressBarStyle.Continuous;
            int v = percent.Value;
            if (v < 0) v = 0;
            if (v > 100) v = 100;
            _progress.Value = v;
        }
        else if (busy)
        {
            _progress.Style = ProgressBarStyle.Marquee;
        }
        else
        {
            _progress.Style = ProgressBarStyle.Continuous;
            _progress.Value = 0;
        }
    }
    void UpdateButtons() {
        bool locked = _busy || _building;
        // Workflow buttons
        _btnDecrypt.Enabled = !locked;
        _btnLoad.Enabled = !locked;
        _btnLift.Enabled = _canLift && !locked;
        _btnBuild.Enabled = _canBuild && !locked;
        _btnRun.Enabled = _canRun && !locked;
        _btnCopy.Enabled = _canCopy && !locked;
        _btnCancel.Enabled = locked || _gameProc != null;
        _btnCancel.Text = _building ? "Stop / Cancel Build" : "Stop Game";
        // Graphics unlocked only after successful Decompile; locked during build
        _gfx.Enabled = _gfxUnlocked && !locked;
        _threads.Enabled = !locked;
        if (_btnRpcs3 != null) _btnRpcs3.Enabled = !locked;
    }
    void SaveSettings() { try { _settings.Save(); } catch { } }
    void UpdateRpcs3Button() {
        if (_btnRpcs3 == null) return;
        bool have = !string.IsNullOrWhiteSpace(_settings.Rpcs3Path) && File.Exists(_settings.Rpcs3Path);
        // Hide when a valid RPCS3.exe is set; show again if missing so user can re-locate
        _btnRpcs3.Visible = !have;
        _btnRpcs3.Text = have ? "RPCS3 set" : "RPCS3 path\u2026";
    }

    /// <summary>On startup: if saved RPCS3 path is missing, warn and show the locate button.</summary>
    void CheckRpcs3OnStartup() {
        if (string.IsNullOrWhiteSpace(_settings.Rpcs3Path)) {
            UpdateRpcs3Button();
            return;
        }
        if (File.Exists(_settings.Rpcs3Path)) {
            UpdateRpcs3Button();
            return;
        }
        Append("RPCS3 path missing: " + _settings.Rpcs3Path);
        MessageBox.Show(this,
            "RPCS3 was previously set but the file is missing:\n\n" +
            _settings.Rpcs3Path + "\n\n" +
            "Please locate RPCS3.exe again (Decrypt EBOOT needs it).",
            "PPSX33 — RPCS3 not found",
            MessageBoxButtons.OK, MessageBoxIcon.Warning);
        _settings.Rpcs3Path = "";
        SaveSettings();
        UpdateRpcs3Button();
    }

    void NotifyBuildComplete(bool success)
    {
        if (IsDisposed) return;
        if (InvokeRequired) { BeginInvoke(() => NotifyBuildComplete(success)); return; }
        try
        {
            if (success)
            {
                MessageBox.Show(this,
                    "Build completed successfully.\r\n\r\n" +
                    "Run and Copy to game folder are now unlocked.\r\n" +
                    "Phase XXIII runtime is used when present under output/src.\r\n" +
                    "Copy places the game EXE and required files next to EBOOT.BIN\r\n" +
                    "(typically ...\\PS3_GAME\\USRDIR).",
                    "PPSX33 - Build complete",
                    MessageBoxButtons.OK,
                    MessageBoxIcon.Information);
            }
            else if (!_buildCancelRequested)
            {
                MessageBox.Show(this,
                    "Build failed. Check the Console tab and build_log for details.",
                    "PPSX33 \u2014 Build failed",
                    MessageBoxButtons.OK,
                    MessageBoxIcon.Error);
            }
        }
        catch { }
    }

    async Task BuildWithProgress() {
        if (_projectDir == null) return;
        _building = true; _buildCancelRequested = false;
        _busy = true; UpdateButtons();
        HideControllerAfterBuild();
        SaveSettings();
        WritePadMapForRuntime();
        string livePath = Path.Combine(_projectDir, "output", "build_live.log");
        try { if (File.Exists(livePath)) File.Delete(livePath); } catch { }

        Native.BeginBuild();
        Append("Build started \u2014 live console follows:");
        SetStatus("Building\u2026", true, 0, "ETA \u2014");

        var start = DateTime.UtcNow;
        long readPos = 0;
        int stepsDone = 0;
        int totalSteps = 0;
        var buildTask = Task.Run(() => Native.Build(_projectDir!, RuntimeDir, CompilersDir, out var l));

        while (!buildTask.IsCompleted) {
            await Task.Delay(400);
            try {
                if (File.Exists(livePath)) {
                    using var fs = new FileStream(livePath, FileMode.Open, FileAccess.Read, FileShare.ReadWrite);
                    if (fs.Length > readPos) {
                        fs.Seek(readPos, SeekOrigin.Begin);
                        using var sr = new StreamReader(fs, Encoding.UTF8);
                        string chunk = await sr.ReadToEndAsync();
                        readPos = fs.Position;
                        if (!string.IsNullOrWhiteSpace(chunk)) {
                            Append(chunk.TrimEnd());
                            foreach (var line in chunk.Split('\n')) {
                                if (line.StartsWith("BUILD_TOTAL=", StringComparison.OrdinalIgnoreCase)) {
                                    if (int.TryParse(line.AsSpan(12).Trim(), out int t) && t > 0) totalSteps = t;
                                }
                                if (line.StartsWith("COMPILE", StringComparison.OrdinalIgnoreCase) ||
                                    line.StartsWith("LINK", StringComparison.OrdinalIgnoreCase) ||
                                    line.StartsWith("cleanup", StringComparison.OrdinalIgnoreCase))
                                    stepsDone++;
                            }
                        }
                    }
                }
            } catch { }

            double elapsed = (DateTime.UtcNow - start).TotalSeconds;
            int denom = totalSteps > 0 ? totalSteps : Math.Max(stepsDone + 1, 1);
            int pct = Math.Clamp((int)(100.0 * stepsDone / denom), 0, 99);
            string eta = "ETA \u2014";
            if (stepsDone >= 2 && elapsed > 1.5) {
                int remain = (int)Math.Max(0, (totalSteps - stepsDone) * (elapsed / stepsDone));
                eta = remain >= 60 ? $"ETA {remain / 60}m {remain % 60}s" : $"ETA {remain}s";
            }
            string totLabel = totalSteps > 0 ? totalSteps.ToString() : "?";
            SetStatus($"Building\u2026 {stepsDone}/{totLabel}", true, pct, eta);
            if (_buildCancelRequested) Native.CancelBuild();
        }

        bool ok = false;
        try { ok = await buildTask; } catch (Exception ex) { Append(ex.Message); }

        try {
            if (File.Exists(livePath)) {
                using var fs = new FileStream(livePath, FileMode.Open, FileAccess.Read, FileShare.ReadWrite);
                if (fs.Length > readPos) {
                    fs.Seek(readPos, SeekOrigin.Begin);
                    using var sr = new StreamReader(fs, Encoding.UTF8);
                    string rest = await sr.ReadToEndAsync();
                    if (!string.IsNullOrWhiteSpace(rest)) Append(rest.TrimEnd());
                }
            }
        } catch { }

        if (ok) {
            _canCopy = true; _canRun = true;
            SetStatus("Building successful", false, 100, "");
            Append("Building successful \u2014 Run and Copy unlocked.");
        } else if (_buildCancelRequested) {
            SetStatus("Build cancelled");
            Append("Build cancelled.");
        } else {
            SetStatus("Building failed");
        }
        _building = false; _busy = false; UpdateButtons();
        NotifyBuildComplete(ok);
        if (ok && _projectDir != null) {
            try {
                var cb = Path.Combine(_projectDir, "codebase");
                var inp = Path.Combine(_projectDir, "input");
                if (Directory.Exists(cb)) Directory.Delete(cb, true);
                if (Directory.Exists(inp)) Directory.Delete(inp, true);
                Append("cleanup: removed project codebase/ and input/ (output/ kept)");
            } catch (Exception ex) { Append("cleanup warn: " + ex.Message); }
        }
    }

    async Task ConfirmCancelBuildAsync() {
        if (!_building) return;
        _buildCancelRequested = true;
        try { _cts?.Cancel(); } catch { }
        try { Native.CancelBuild(); } catch (Exception ex) { Append("Cancel signal: " + ex.Message); }
        Append("Build cancel requested \u2014 stopping compiler (project folder kept)\u2026");
        SetStatus("Cancelling build\u2026", true);
        await Task.Delay(800);
        // Keep the form open — never Exit / Close here
        _building = false;
        _busy = false;
        UpdateButtons();
        Append("Build cancelled. Project folder preserved — you can Build again.");
        SetStatus("Build cancelled");
    }

    async Task RunStep(string status, Func<CancellationToken, bool> work, bool afterLift = false, bool afterBuild = false) {
        if (_projectDir == null) return;
        _cts = new CancellationTokenSource();
        _busy = true; UpdateButtons(); SetStatus(status, true); SaveSettings();
        bool ok = false;
        try { ok = await Task.Run(() => work(_cts.Token)); }
        catch (Exception ex) { Append(ex.Message); }
        finally {
            if (afterLift && ok) { _canBuild = true; _gfxUnlocked = true; _liftChunks = Math.Max(1, _liftChunks); Append("Lift chunks tracked: " + _liftChunks); }
            if (afterBuild && ok) { _canCopy = true; _canRun = true; }
            string workName = status.Contains("Decompil", StringComparison.OrdinalIgnoreCase) ? "Decompiling" : status.Contains("Build", StringComparison.OrdinalIgnoreCase) ? "Building" : status.TrimEnd('\u2026', '.', ' ');
            Append(ok ? workName + " successful." : workName + " failed.");
            if (ok && afterLift) Append("Decompiling successful \u2014 Build and Graphics backend unlocked.");
            if (ok && afterBuild) Append("Building successful \u2014 Run and Copy unlocked.");
            _busy = false; UpdateButtons();
            SetStatus(ok ? workName + " done" : workName + " failed");
        }
    }
}
