using System.Diagnostics;
using System.Runtime.InteropServices;
using System.Text;
using System.Text.Json;
using Microsoft.Web.WebView2.Core;
using Microsoft.Web.WebView2.WinForms;

namespace PS3Recomp.Gui;

public sealed partial class MainForm : Form
{
    static readonly Color Bg = Color.FromArgb(9, 13, 20);
    static readonly Color Panel = Color.FromArgb(13, 17, 23);
    static readonly Color Panel2 = Color.FromArgb(22, 27, 34);
    static readonly Color TextPri = Color.FromArgb(230, 237, 243);
    static readonly Color TextSec = Color.FromArgb(100, 116, 139);
    static readonly Color Accent = Color.FromArgb(56, 189, 248);
    static readonly Color Danger = Color.FromArgb(218, 54, 51);

    private readonly Settings _settings = Settings.Load();
    private readonly TextBox _log = new() { Visible = false };
    private readonly TextBox _report = new() { Visible = false };
    private readonly TextBox _roadmap = new() { Visible = false };
    private readonly TextBox _requirements = new() { Visible = false };
    private readonly Ps3HudLabel _project = new() { Visible = false };
    private readonly Label _status = new() { Visible = false };
    private readonly ProgressBar _progress = new() { Visible = false };
    private readonly Ps3Button _btnDecrypt = new() { Visible = false };
    private readonly Ps3Button _btnLoad = new() { Visible = false };
    private readonly Ps3Button _btnLift = new() { Visible = false };
    private readonly Ps3Button _btnBuild = new() { Visible = false };
    private readonly Ps3Button _btnRun = new() { Visible = false };
    private readonly Ps3Button _btnCopy = new() { Visible = false };
    private readonly Ps3Button _btnCancel = new() { Visible = false };
    private readonly ComboBox _gfx = new() { Visible = false };
    private readonly ComboBox _threads = new() { Visible = false };
    private readonly TabControl _tabs = new() { Visible = false };
    private readonly PictureBox _logo = new() { Visible = false };
    private readonly Label _eta = new() { Visible = false };
    private Ps3Button? _btnRpcs3;
    private WebView2? _web;
    private bool _webReady;

    private string? _projectDir;
    private CancellationTokenSource? _cts;
    private Process? _gameProc;
    private bool _building;
    private bool _buildCancelRequested;
    private bool _canLift, _canBuild, _canCopy, _canRun, _gfxUnlocked, _busy;
    private int _liftChunks;
    private bool _controllerMapped;
    private bool _controllerHiddenAfterBuild;

    private static string RuntimeDir => Path.Combine(AppContext.BaseDirectory, "runtime");
    private static string CompilersDir => Path.Combine(AppContext.BaseDirectory, "redist");
    private static string ProjectsRoot => Path.Combine(AppContext.BaseDirectory, "projects");

    [DllImport("dwmapi.dll")]
    private static extern int DwmSetWindowAttribute(IntPtr hwnd, int attr, ref int value, int size);

    public MainForm()
    {
        Text = "PPSX33 · PlayStation 3 Recompiler";
        StartPosition = FormStartPosition.CenterScreen;
        Size = new Size(1200, 780);
        MinimumSize = new Size(950, 650);
        BackColor = Bg;
        ForeColor = TextPri;
        Font = new Font("Segoe UI", 9f);
        DoubleBuffered = true;

        try
        {
            Load += (_, _) =>
            {
                int useDark = 1;
                DwmSetWindowAttribute(Handle, 20, ref useDark, sizeof(int));
            };
        }
        catch { }

        try
        {
            foreach (var ico in new[]
            {
                Path.Combine(AppContext.BaseDirectory, "assets", "ppsx33-logo.ico"),
                Path.Combine(AppContext.BaseDirectory, "ppsx33-logo.ico"),
            })
            {
                if (!File.Exists(ico)) continue;
                Icon = new Icon(ico);
                break;
            }
        }
        catch { }

        // Hidden control surface for partials / controller tab host
        var hidden = new Panel { Visible = false, Size = new Size(1, 1) };
        _btnRpcs3 = new Ps3Button { Visible = false };
        _gfx.Items.AddRange(new object[] { "D3D11 (DirectX 11)", "OpenGL 3.3 (future)", "Vulkan (not linked)" });
        _gfx.SelectedIndex = 0;
        for (int i = 1; i <= Environment.ProcessorCount; i++) _threads.Items.Add(i.ToString());
        _threads.SelectedItem = Math.Min(4, Environment.ProcessorCount).ToString();
        _requirements.Text = BuildRequirementsText();
        _roadmap.Text = BuildRoadmapText();
        hidden.Controls.Add(_tabs);
        Controls.Add(hidden);
        BuildControllerTab();

        _web = new WebView2 { Dock = DockStyle.Fill };
        Controls.Add(_web);
        _web.BringToFront();
        Load += async (_, _) => await InitWebAsync();

        Shown += (_, _) => CheckRpcs3OnStartup();
        FormClosing += (_, _) => CleanupAllProjectWorkFolders();
    }

    async Task InitWebAsync()
    {
        if (_web == null) return;
        try
        {
            await _web.EnsureCoreWebView2Async();
            _web.CoreWebView2.Settings.AreDefaultContextMenusEnabled = false;
            _web.CoreWebView2.Settings.IsStatusBarEnabled = false;
            _web.CoreWebView2.WebMessageReceived += OnWebMessage;
            string www = Path.Combine(AppContext.BaseDirectory, "www", "index.html");
            if (!File.Exists(www))
                www = Path.Combine(AppContext.BaseDirectory, "src", "gui", "www", "index.html");
            if (File.Exists(www))
                _web.Source = new Uri(www);
            else
            {
                _web.NavigateToString("<html><body style='background:#090d14;color:#e6edf3;font-family:sans-serif;padding:24px'>UI www/index.html missing.</body></html>");
            }
            _webReady = true;
            Append("PPSX33 ready · core v1.0.0");
            Append("Settings: " + Settings.DataDir);
            Append("Host build uses system Visual Studio (redist/ = installer + guide only).");
            PushState();
            PostState(new Dictionary<string, object?>
            {
                ["roadmap"] = _roadmap.Text,
                ["requirements"] = _requirements.Text
            });
        }
        catch (Exception ex)
        {
            MessageBox.Show(this,
                "WebView2 failed to start. Install the Evergreen WebView2 Runtime.\n\n" + ex.Message,
                "PPSX33", MessageBoxButtons.OK, MessageBoxIcon.Error);
        }
    }

    void OnWebMessage(object? sender, CoreWebView2WebMessageReceivedEventArgs e)
    {
        string raw = e.TryGetWebMessageAsString() ?? e.WebMessageAsJson;
        try
        {
            using var doc = JsonDocument.Parse(raw);
            var root = doc.RootElement;
            string action = root.TryGetProperty("action", out var a) ? a.GetString() ?? "" : "";
            BeginInvoke(async () => await HandleUiAction(action, root));
        }
        catch { }
    }

    async Task HandleUiAction(string action, JsonElement root)
    {
        switch (action)
        {
            case "ready":
                PushState();
                break;
            case "decrypt":
                await DecryptEbootAndLoad();
                break;
            case "load":
            case "browse":
                LoadElf(null);
                break;
            case "decompile":
                await RunStep("Decompiling", ct => Native.Lift(_projectDir!, out _), afterLift: true);
                break;
            case "build":
                await BuildWithProgress();
                break;
            case "run":
                await RunGame();
                break;
            case "copy":
                CopyToGameFolder();
                break;
            case "cancel":
                if (_building && !_buildCancelRequested) await ConfirmCancelBuildAsync();
                else StopGame();
                break;
            case "rpcs3":
                LocateRpcs3();
                break;
            case "controller":
                if (_controllerTab != null)
                {
                    // Show classic controller tab in a dialog-sized form host
                    using var f = new Form
                    {
                        Text = "Controller mapping",
                        Size = new Size(520, 480),
                        StartPosition = FormStartPosition.CenterParent,
                        BackColor = Panel
                    };
                    if (_controllerPanel != null)
                    {
                        var p = _controllerPanel;
                        if (p.Parent != null) p.Parent.Controls.Remove(p);
                        p.Dock = DockStyle.Fill;
                        p.Visible = true;
                        f.Controls.Add(p);
                        f.ShowDialog(this);
                        // re-home
                        f.Controls.Remove(p);
                        if (_controllerTab != null)
                        {
                            _controllerTab.Controls.Add(p);
                        }
                    }
                }
                break;
            case "gfx":
                if (root.TryGetProperty("value", out var gv))
                {
                    string v = gv.GetString() ?? "d3d11";
                    int idx = v switch { "ogl" => 1, "vk" => 2, _ => 0 };
                    if (idx < _gfx.Items.Count) _gfx.SelectedIndex = idx;
                    Append("Graphics API set: " + _gfx.SelectedItem);
                }
                break;
            case "threads":
                if (root.TryGetProperty("value", out var tv) && int.TryParse(tv.GetString(), out int n))
                {
                    string s = n.ToString();
                    if (_threads.Items.Contains(s)) _threads.SelectedItem = s;
                    Append("CPU threads: " + s);
                }
                break;
        }
        PushState();
    }

    void LocateRpcs3()
    {
        using var ofd = new OpenFileDialog
        {
            Title = "Locate RPCS3.exe",
            Filter = "RPCS3|RPCS3.exe|Executables|*.exe|All|*.*"
        };
        if (ofd.ShowDialog(this) != DialogResult.OK) return;
        _settings.Rpcs3Path = ofd.FileName;
        SaveSettings();
        UpdateRpcs3Button();
        Append("RPCS3 path set: " + _settings.Rpcs3Path);
    }

    void PostToWeb(object payload)
    {
        if (_web?.CoreWebView2 == null || !_webReady) return;
        try
        {
            string json = JsonSerializer.Serialize(payload);
            if (InvokeRequired) BeginInvoke(() => _web.CoreWebView2.PostWebMessageAsJson(json));
            else _web.CoreWebView2.PostWebMessageAsJson(json);
        }
        catch { }
    }

    void PostState(Dictionary<string, object?> extra)
    {
        var d = new Dictionary<string, object?>(extra) { ["type"] = "state" };
        PostToWeb(d);
    }

    void PushState()
    {
        bool locked = _busy || _building;
        PostState(new Dictionary<string, object?>
        {
            ["projectPath"] = string.IsNullOrEmpty(_projectDir) ? (_project.Text ?? "No project loaded") : _projectDir,
            ["status"] = _status.Text ?? "Ready",
            ["header"] = locked ? "WORKING" : "ENGINE READY",
            ["percent"] = _progress.Style == ProgressBarStyle.Continuous ? _progress.Value : 0,
            ["cancelLabel"] = _building ? "Stop / Cancel Build" : "Stop Game",
            ["enable"] = new Dictionary<string, bool>
            {
                ["decompile"] = _canLift && !locked,
                ["build"] = _canBuild && !locked,
                ["run"] = _canRun && !locked,
                ["copy"] = _canCopy && !locked,
                ["cancel"] = locked || _gameProc != null,
                ["gfx"] = _gfxUnlocked && !locked
            }
        });
    }

    static string BuildRequirementsText() =>
        "PPSX33 — Requirements\r\n" +
        "Required: Win10/11 x64, D3D11 FL11_0, 8GB RAM, system Visual Studio Community\r\n" +
        "Recommended: 16GB RAM, discrete GPU, 2GB+ VRAM, SSD\r\n" +
        "redist/ ships VS installer + guide only — no bundled compilers.\r\n";

    static string BuildRoadmapText() =>
        "PPSX33 Roadmap\r\n" +
        "PHASE 1–XXII core pipeline\r\n" +
        "XXIII execution reality / guest FIFO / draw path\r\n" +
        "Workflow: Decrypt/Load → Decompile → Build → Copy → Run\r\n";

    void CleanupAllProjectWorkFolders()
    {
        try
        {
            if (!Directory.Exists(ProjectsRoot)) return;
            foreach (var dir in Directory.GetDirectories(ProjectsRoot))
            {
                try
                {
                    var codebase = Path.Combine(dir, "codebase");
                    var input = Path.Combine(dir, "input");
                    if (Directory.Exists(codebase)) Directory.Delete(codebase, true);
                    if (Directory.Exists(input)) Directory.Delete(input, true);
                }
                catch { }
            }
        }
        catch { }
    }
}
