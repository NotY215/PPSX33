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

    void BuildControllerTab()
    {
        if (_controllerTab != null) return;

        _controllerPanel = new Panel
        {
            Dock = DockStyle.Fill,
            BackColor = Color.FromArgb(9, 13, 22)
        };

        _controllerWeb = new WebView2 { Dock = DockStyle.Fill };
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
            await _controllerWeb.EnsureCoreWebView2Async();
            _controllerWeb.CoreWebView2.Settings.AreDefaultContextMenusEnabled = false;
            _controllerWeb.CoreWebView2.Settings.IsStatusBarEnabled = false;
            _controllerWeb.CoreWebView2.WebMessageReceived += ControllerWeb_Message;
            string htmlPath = Path.Combine(AppContext.BaseDirectory, "www", "controller.html");
            if (!File.Exists(htmlPath))
                htmlPath = Path.Combine(AppContext.BaseDirectory, "src", "gui", "www", "controller.html");
            if (File.Exists(htmlPath))
                _controllerWeb.Source = new Uri(htmlPath);
            else
            {
                // navigate to embedded minimal note
                _controllerWeb.NavigateToString("<html><body style='background:#090d16;color:#fff;font-family:sans-serif;padding:24px'>controller.html missing from output www/</body></html>");
            }
            _controllerWebReady = true;
            PushKeyMapToWeb();
        }
        catch (Exception ex)
        {
            Append("Controller WebView2 failed: " + ex.Message + " (install WebView2 Runtime)");
            // fallback label
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
            string json = e.TryGetWebMessageAsString() ?? e.WebMessageAsJson;
            using var doc = JsonDocument.Parse(json);
            var root = doc.RootElement;
            string type = root.TryGetProperty("type", out var t) ? t.GetString() ?? "" : "";
            if (type == "ready")
            {
                BeginInvoke(PushKeyMapToWeb);
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
            if (type == "bind" && root.TryGetProperty("id", out var idEl) && root.TryGetProperty("key", out var keyEl))
            {
                ApplyOneBind(idEl.GetString() ?? "", keyEl.GetString() ?? "");
                _settings.KeyMap ??= KeyMap.Defaults();
                SaveSettings();
            }
        }
        catch (Exception ex)
        {
            BeginInvoke(() => Append("Controller message error: " + ex.Message));
        }
    }

    void PushKeyMapToWeb()
    {
        if (_controllerWeb?.CoreWebView2 == null || !_controllerWebReady) return;
        var km = _settings.KeyMap ?? KeyMap.Defaults();
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
        var payload = JsonSerializer.Serialize(new { type = "setMap", map });
        try { _controllerWeb.CoreWebView2.PostWebMessageAsJson(payload); } catch { }
    }

    void ApplyMapElement(JsonElement mapEl)
    {
        var km = _settings.KeyMap ?? KeyMap.Defaults();
        string G(string id) => mapEl.TryGetProperty(id, out var v) ? (v.GetString() ?? "") : "";
        km.DpadUp = G("dpad_up"); km.DpadDown = G("dpad_down");
        km.DpadLeft = G("dpad_left"); km.DpadRight = G("dpad_right");
        km.Triangle = G("triangle"); km.Circle = G("circle");
        km.Cross = G("cross"); km.Square = G("square");
        km.L1 = G("l1"); km.R1 = G("r1"); km.L2 = G("l2"); km.R2 = G("r2");
        km.Select = G("select"); km.Start = G("start"); km.PsButton = G("ps");
        km.LeftStickUp = G("ls_up"); km.LeftStickDown = G("ls_down");
        km.LeftStickLeft = G("ls_left"); km.LeftStickRight = G("ls_right");
        km.RightStickUp = G("rs_up"); km.RightStickDown = G("rs_down");
        km.RightStickLeft = G("rs_left"); km.RightStickRight = G("rs_right");
        km.L3 = G("l3"); km.R3 = G("r3");
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
