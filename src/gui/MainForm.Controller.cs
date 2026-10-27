using System.Text.Json;
using Microsoft.Web.WebView2.Core;
using Microsoft.Web.WebView2.WinForms;

namespace PS3Recomp.Gui;

public sealed partial class MainForm
{
    private Panel? _controllerPanel;
    private TabPage? _controllerTab;
    private WebView2? _controllerView;
    private bool _controllerReady;

    void BuildControllerTab()
    {
        if (_controllerTab != null) return;

        _controllerPanel = new Panel { Dock = DockStyle.Fill, BackColor = Color.FromArgb(4, 8, 16), Padding = new Padding(0) };
        _controllerView = new WebView2 { Dock = DockStyle.Fill, DefaultBackgroundColor = Color.FromArgb(4, 8, 16) };
        _controllerView.CoreWebView2InitializationCompleted += (_, e) =>
        {
            if (!e.IsSuccess || _controllerView?.CoreWebView2 == null)
            {
                Append("Controller HTML initialization failed: " + e.InitializationException?.Message);
                return;
            }
            var core = _controllerView.CoreWebView2;
            core.Settings.AreDefaultContextMenusEnabled = false;
            core.Settings.AreDevToolsEnabled = false;
            core.WebMessageReceived += ControllerWebMessageReceived;
            string htmlPath = Path.Combine(AppContext.BaseDirectory, "www", "controller.html");
            if (File.Exists(htmlPath))
                core.Navigate(new Uri(htmlPath).AbsoluteUri);
            else
                Append("Controller HTML file not found: " + htmlPath);
        };
        _controllerView.NavigationCompleted += async (_, e) =>
        {
            if (!e.IsSuccess || _controllerView?.CoreWebView2 == null) return;
            _controllerReady = true;
            string json = JsonSerializer.Serialize(_settings.KeyMap ?? KeyMap.Defaults());
            await _controllerView.CoreWebView2.ExecuteScriptAsync("window.loadMap && window.loadMap(" + json + ");");
        };
        _controllerPanel.Controls.Add(_controllerView);
        _controllerTab = new TabPage("Controller") { BackColor = Color.FromArgb(4, 8, 16), Padding = new Padding(0) };
        _controllerTab.Controls.Add(_controllerPanel);
        _tabs.TabPages.Add(_controllerTab);
        _ = InitializeControllerWebViewAsync();
    }

    private async Task InitializeControllerWebViewAsync()
    {
        try
        {
            if (_controllerView != null)
                await _controllerView.EnsureCoreWebView2Async();
        }
        catch (Exception ex) { Append("Controller WebView2 failed to initialize: " + ex.Message); }
    }

    private void ControllerWebMessageReceived(object? sender, CoreWebView2WebMessageReceivedEventArgs e)
    {
        try
        {
            using var doc = JsonDocument.Parse(e.WebMessageAsJson);
            string type = doc.RootElement.TryGetProperty("type", out var t) ? t.GetString() ?? "" : "";
            if (!doc.RootElement.TryGetProperty("map", out var mapElement)) return;
            var imported = JsonSerializer.Deserialize<KeyMap>(mapElement.GetRawText());
            if (imported == null) return;
            _settings.KeyMap = imported;
            if (type == "changed")
            {
                SaveSettings();
                _controllerMapped = false;
            }
            else if (type == "save")
            {
                SaveSettings();
                WritePadMapForRuntime();
                _controllerMapped = true;
                Append("Controller mapping saved from HTML Controller Studio.");
                UpdateButtons();
            }
        }
        catch (Exception ex) { Append("Controller message rejected: " + ex.Message); }
    }

    void WritePadMapForRuntime()
    {
        try
        {
            if (string.IsNullOrEmpty(_projectDir))
            {
                Append("Key map saved to settings; load a project to also write output/pad_map.json.");
                return;
            }
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
