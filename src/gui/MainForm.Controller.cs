using System.Text.Json;
using Microsoft.Web.WebView2.Core;
using Microsoft.Web.WebView2.WinForms;

namespace PS3Recomp.Gui;

public sealed partial class MainForm
{
    private Panel? _controllerPanel;
    private TabPage? _controllerTab;
    private WebView2? _controllerWeb;
    private bool _controllerWebReady;
    private bool _controllerNavDone;

    void BuildControllerTab()
    {
        if (_controllerTab != null) return;

        _controllerPanel = new Panel
        {
            Dock = DockStyle.Fill,
            BackColor = Color.FromArgb(9, 13, 22)
        };

        _controllerWeb = new WebView2
        {
            Dock = DockStyle.Fill,
            DefaultBackgroundColor = Color.FromArgb(9, 13, 22)
        };
        _controllerPanel.Controls.Add(_controllerWeb);

        _controllerTab = new TabPage("Controller")
        {
            BackColor = Color.FromArgb(9, 13, 22),
            Padding = new Padding(0)
        };
        _controllerTab.Controls.Add(_controllerPanel);
        _tabs.TabPages.Add(_controllerTab);

        _ = InitControllerWebAsync();
    }

    async Task InitControllerWebAsync()
    {
        if (_controllerWeb == null) return;
        try
        {
            var env = await CoreWebView2Environment.CreateAsync(null, Path.Combine(Path.GetTempPath(), "PPSX33WebView2"));
            await _controllerWeb.EnsureCoreWebView2Async(env);
            _controllerWeb.CoreWebView2.Settings.AreDefaultContextMenusEnabled = false;
            _controllerWeb.CoreWebView2.Settings.IsStatusBarEnabled = false;
            _controllerWeb.CoreWebView2.Settings.AreDevToolsEnabled = false;
            // Disable smooth scroll animation thrash in embedded host
            try { await _controllerWeb.CoreWebView2.ExecuteScriptAsync("document.documentElement.style.scrollBehavior='auto'"); } catch { }

            _controllerWeb.CoreWebView2.WebMessageReceived += ControllerWeb_Message;
            _controllerWeb.CoreWebView2.NavigationCompleted += (_, args) =>
            {
                _controllerNavDone = args.IsSuccess;
                if (args.IsSuccess)
                    BeginInvoke(() => { _controllerWebReady = true; PushKeyMapToWeb(); });
            };

            string htmlPath = Path.Combine(AppContext.BaseDirectory, "www", "controller.html");
            if (!File.Exists(htmlPath))
                htmlPath = Path.Combine(AppContext.BaseDirectory, "src", "gui", "www", "controller.html");

            if (File.Exists(htmlPath))
            {
                // file:/// URI — avoid invalid range from relative Source
                var uri = new Uri(htmlPath);
                _controllerWeb.CoreWebView2.Navigate(uri.AbsoluteUri);
            }
            else
            {
                _controllerWeb.NavigateToString("<html><body style='background:#090d16;color:#fff;font-family:sans-serif;padding:24px'>controller.html missing from output www/</body></html>");
                _controllerWebReady = true;
            }
        }
        catch (Exception ex)
        {
            Append("Controller WebView2 failed: " + ex.Message + " (install WebView2 Runtime)");
            var lab = new Label
            {
                Dock = DockStyle.Fill,
                TextAlign = ContentAlignment.MiddleCenter,
                ForeColor = Color.Orange,
                Text = "WebView2 required for DualShock mapper.\n" + ex.Message
            };
            _controllerPanel?.Controls.Add(lab);
        }
    }

    void ControllerWeb_Message(object? sender, CoreWebView2WebMessageReceivedEventArgs e)
    {
        try
        {
            // Object postMessage → WebMessageAsJson. TryGetWebMessageAsString throws
            // "Value does not fall within the expected range" when payload is not a string.
            string json;
            try { json = e.WebMessageAsJson; }
            catch
            {
                try { json = e.TryGetWebMessageAsString(); }
                catch (Exception ex2)
                {
                    BeginInvoke(() => Append("Controller message error: " + ex2.Message));
                    return;
                }
            }
            if (string.IsNullOrWhiteSpace(json)) return;

            using var doc = JsonDocument.Parse(json);
            var root = doc.RootElement;
            // Some hosts wrap once
            if (root.ValueKind == JsonValueKind.String)
            {
                var inner = root.GetString();
                if (string.IsNullOrEmpty(inner)) return;
                using var doc2 = JsonDocument.Parse(inner);
                root = doc2.RootElement.Clone();
            }

            string type = root.TryGetProperty("type", out var t) ? t.GetString() ?? "" : "";
            if (type == "ready")
            {
                if (_controllerNavDone) BeginInvoke(PushKeyMapToWeb);
                return;
            }
            if (type == "save" && root.TryGetProperty("map", out var mapEl))
            {
                ApplyMapElement(mapEl);
                WritePadMapForRuntime();
                _controllerMapped = true;
                BeginInvoke(() => { Append("Controller mapping saved from WebView."); UpdateButtons(); });
                return;
            }
            if (type == "bind")
            {
                string id = root.TryGetProperty("id", out var idEl) ? idEl.GetString() ?? "" : "";
                string key = root.TryGetProperty("key", out var keyEl) ? keyEl.GetString() ?? "" : "";
                if (!string.IsNullOrEmpty(id) && !string.IsNullOrEmpty(key))
                {
                    ApplyOneBind(id, key);
                    _settings.KeyMap ??= KeyMap.Defaults();
                    SaveSettings();
                }
            }
        }
        catch (Exception ex)
        {
            BeginInvoke(() => Append("Controller message error: " + ex.Message));
        }
    }

    void PushKeyMapToWeb()
    {
        if (_controllerWeb?.CoreWebView2 == null || !_controllerWebReady || !_controllerNavDone) return;
        var km = _settings.KeyMap ?? KeyMap.Defaults();
        // Prefer distinct L/R modifier names so HTML can match by code
        var map = new Dictionary<string, string>
        {
            ["dpad_up"] = km.DpadUp, ["dpad_down"] = km.DpadDown,
            ["dpad_left"] = km.DpadLeft, ["dpad_right"] = km.DpadRight,
            ["triangle"] = km.Triangle, ["circle"] = km.Circle,
            ["cross"] = km.Cross, ["square"] = km.Square,
            ["l1"] = km.L1, ["r1"] = km.R1, ["l2"] = km.L2, ["r2"] = km.R2,
            ["select"] = km.Select, ["start"] = km.Start, ["ps"] = km.PsButton,
            ["ls_up"] = km.LeftStickUp, ["ls_down"] = km.LeftStickDown,
            ["ls_left"] = km.LeftStickLeft, ["ls_right"] = km.LeftStickRight,
            ["rs_up"] = km.RightStickUp, ["rs_down"] = km.RightStickDown,
            ["rs_left"] = km.RightStickLeft, ["rs_right"] = km.RightStickRight,
            ["l3"] = km.L3, ["r3"] = km.R3
        };
        try
        {
            var payload = JsonSerializer.Serialize(new { type = "setMap", map });
            _controllerWeb.CoreWebView2.PostWebMessageAsJson(payload);
        }
        catch (Exception ex)
        {
            Append("Controller push map failed: " + ex.Message);
        }
    }

    void ApplyMapElement(JsonElement mapEl)
    {
        var km = _settings.KeyMap ?? KeyMap.Defaults();
        string G(string id) => mapEl.TryGetProperty(id, out var v) ? (v.GetString() ?? "") : "";
        void Set(string id, Action<string> set)
        {
            var s = G(id);
            if (!string.IsNullOrEmpty(s)) set(s);
        }
        Set("dpad_up", s => km.DpadUp = s);
        Set("dpad_down", s => km.DpadDown = s);
        Set("dpad_left", s => km.DpadLeft = s);
        Set("dpad_right", s => km.DpadRight = s);
        Set("triangle", s => km.Triangle = s);
        Set("circle", s => km.Circle = s);
        Set("cross", s => km.Cross = s);
        Set("square", s => km.Square = s);
        Set("l1", s => km.L1 = s);
        Set("r1", s => km.R1 = s);
        Set("l2", s => km.L2 = s);
        Set("r2", s => km.R2 = s);
        Set("select", s => km.Select = s);
        Set("start", s => km.Start = s);
        Set("ps", s => km.PsButton = s);
        Set("ls_up", s => km.LeftStickUp = s);
        Set("ls_down", s => km.LeftStickDown = s);
        Set("ls_left", s => km.LeftStickLeft = s);
        Set("ls_right", s => km.LeftStickRight = s);
        Set("rs_up", s => km.RightStickUp = s);
        Set("rs_down", s => km.RightStickDown = s);
        Set("rs_left", s => km.RightStickLeft = s);
        Set("rs_right", s => km.RightStickRight = s);
        Set("l3", s => km.L3 = s);
        Set("r3", s => km.R3 = s);
        _settings.KeyMap = km;
        SaveSettings();
    }

    void ApplyOneBind(string id, string key)
    {
        if (string.IsNullOrEmpty(id) || string.IsNullOrEmpty(key)) return;
        var km = _settings.KeyMap ?? KeyMap.Defaults();
        switch (id)
        {
            case "dpad_up": km.DpadUp = key; break;
            case "dpad_down": km.DpadDown = key; break;
            case "dpad_left": km.DpadLeft = key; break;
            case "dpad_right": km.DpadRight = key; break;
            case "triangle": km.Triangle = key; break;
            case "circle": km.Circle = key; break;
            case "cross": km.Cross = key; break;
            case "square": km.Square = key; break;
            case "l1": km.L1 = key; break;
            case "r1": km.R1 = key; break;
            case "l2": km.L2 = key; break;
            case "r2": km.R2 = key; break;
            case "select": km.Select = key; break;
            case "start": km.Start = key; break;
            case "ps": km.PsButton = key; break;
            case "ls_up": km.LeftStickUp = key; break;
            case "ls_down": km.LeftStickDown = key; break;
            case "ls_left": km.LeftStickLeft = key; break;
            case "ls_right": km.LeftStickRight = key; break;
            case "rs_up": km.RightStickUp = key; break;
            case "rs_down": km.RightStickDown = key; break;
            case "rs_left": km.RightStickLeft = key; break;
            case "rs_right": km.RightStickRight = key; break;
            case "l3": km.L3 = key; break;
            case "r3": km.R3 = key; break;
        }
        _settings.KeyMap = km;
    }

    void WritePadMapForRuntime()
    {
        try
        {
            if (string.IsNullOrEmpty(_projectDir)) return;
            string outDir = Path.Combine(_projectDir, "output");
            Directory.CreateDirectory(outDir);
            var km = _settings.KeyMap ?? KeyMap.Defaults();
            string path = Path.Combine(outDir, "pad_map.json");
            File.WriteAllText(path, JsonSerializer.Serialize(km, new JsonSerializerOptions { WriteIndented = true }));
            Append("Wrote pad_map.json → " + path);
        }
        catch (Exception ex) { Append("pad_map write failed: " + ex.Message); }
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
}
