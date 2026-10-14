using System.Diagnostics;
using System.Text;

namespace PS3Recomp.Gui;

public sealed partial class MainForm
{
    void AddTab(string title, Control body) { var page = new TabPage(title) { BackColor = Panel, Padding = new Padding(4) }; page.Controls.Add(body); _tabs.TabPages.Add(page); }

    void StyleBtn(Control b, string text, Color bg) { b.Text = text; b.BackColor = bg; b.ForeColor = TextPri; }
    void StyleTextBox(TextBox t) { t.Multiline = true; t.ScrollBars = ScrollBars.Both; t.Dock = DockStyle.Fill; t.BackColor = Panel; t.ForeColor = TextPri; t.BorderStyle = BorderStyle.None; t.Font = new Font("Consolas", 9f); }
    Label MakeLbl(string text, Color c, bool bold) => new Label { Text = text, ForeColor = c, BackColor = Color.Transparent, Font = bold ? new Font("Segoe UI", 9f, FontStyle.Bold) : new Font("Segoe UI", 9f) };
    void DrawPs3Tab(object? sender, DrawItemEventArgs e) {
        if (e.Index < 0) return;
        var g = e.Graphics;
        var tab = _tabs.TabPages[e.Index];
        bool sel = (e.State & DrawItemState.Selected) != 0;
        using var br = new SolidBrush(sel ? Panel2 : Panel);
        g.FillRectangle(br, e.Bounds);
        TextRenderer.DrawText(g, tab.Text, Font, e.Bounds, sel ? TextPri : TextSec, TextFormatFlags.HorizontalCenter | TextFormatFlags.VerticalCenter);
    }
    void Append(string s) {
        if (IsDisposed) return;
        if (InvokeRequired) { BeginInvoke(() => Append(s)); return; }
        _log.AppendText(s.TrimEnd() + Environment.NewLine);
    }
    void SetStatus(string s, bool busy = false, int? percent = null, string? eta = null) {
        if (InvokeRequired) { BeginInvoke(() => SetStatus(s, busy, percent, eta)); return; }
        _status.Text = string.IsNullOrEmpty(eta) ? s : s + "  ·  " + eta;
        if (percent.HasValue) { _progress.Style = ProgressBarStyle.Continuous; _progress.Value = Math.Clamp(percent.Value, 0, 100); }
        else if (busy) _progress.Style = ProgressBarStyle.Marquee;
        else { _progress.Style = ProgressBarStyle.Continuous; _progress.Value = 0; }
    }
    void UpdateButtons() {
        _btnLift.Enabled = _canLift && !_busy;
        _btnBuild.Enabled = _canBuild && !_busy;
        _btnRun.Enabled = _canRun && !_busy;
        _btnCopy.Enabled = _canCopy && !_busy;
        _btnCancel.Enabled = _busy || _building || _gameProc != null;
        _btnCancel.Text = _building ? "Stop / Cancel Build" : "Stop Game";
        if (_gfxUnlocked) { }
    }
    void SaveSettings() { try { _settings.Save(); } catch { } }
    void UpdateRpcs3Button() {
        if (_btnRpcs3 == null) return;
        _btnRpcs3.Text = string.IsNullOrEmpty(_settings.Rpcs3Path) ? "RPCS3 path…" : "RPCS3 set";
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
        Append("Build started — live console follows:");
        SetStatus("Building…", true, 0, "ETA —");

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
            string eta = "ETA —";
            if (stepsDone >= 2 && elapsed > 1.5) {
                int remain = (int)Math.Max(0, (totalSteps - stepsDone) * (elapsed / stepsDone));
                eta = remain >= 60 ? $"ETA {remain / 60}m {remain % 60}s" : $"ETA {remain}s";
            }
            string totLabel = totalSteps > 0 ? totalSteps.ToString() : "?";
            SetStatus($"Building… {stepsDone}/{totLabel}", true, pct, eta);
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
            Append("Building successful — Run and Copy unlocked.");
        } else if (_buildCancelRequested) {
            SetStatus("Build cancelled");
        } else {
            SetStatus("Building failed");
        }
        _building = false; _busy = false; UpdateButtons();
    }

    async Task ConfirmCancelBuildAsync() {
        _buildCancelRequested = true;
        _cts?.Cancel();
        Native.CancelBuild();
        _building = false; _busy = false; UpdateButtons();
        Append("Build cancel requested…");
        await Task.CompletedTask;
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
            string workName = status.Contains("Decompil", StringComparison.OrdinalIgnoreCase) ? "Decompiling" : status.Contains("Build", StringComparison.OrdinalIgnoreCase) ? "Building" : status.TrimEnd('…', '.', ' ');
            Append(ok ? workName + " successful." : workName + " failed.");
            if (ok && afterLift) Append("Decompiling successful — Build and Graphics backend unlocked.");
            if (ok && afterBuild) Append("Building successful — Run and Copy unlocked.");
            _busy = false; UpdateButtons();
            SetStatus(ok ? workName + " done" : workName + " failed");
        }
    }
}
