using System.Diagnostics;
using System.Runtime.InteropServices;
using System.Text;
using System.Drawing.Drawing2D;

namespace PS3Recomp.Gui;

public sealed partial class MainForm : Form
{
    static readonly Color Bg = Color.FromArgb(6, 8, 18);
    static readonly Color Panel = Color.FromArgb(14, 20, 44);
    static readonly Color Panel2 = Color.FromArgb(20, 30, 60);
    static readonly Color TextPri = Color.FromArgb(238, 242, 255);
    static readonly Color TextSec = Color.FromArgb(150, 162, 196);
    static readonly Color Accent = Color.FromArgb(38, 112, 210);
    static readonly Color Danger = Color.FromArgb(210, 55, 78);

    private readonly Settings _settings = Settings.Load();
    private readonly TextBox _log = new();
    private readonly TextBox _report = new();
    private readonly TextBox _roadmap = new();
    private readonly TextBox _requirements = new();
    private readonly Ps3HudLabel _project = new();
    private readonly Label _status = new();
    private readonly ProgressBar _progress = new();
    private readonly Ps3Button _btnDecrypt = new();
    private readonly Ps3Button _btnLoad = new();
    private readonly Ps3Button _btnLift = new();
    private readonly Ps3Button _btnBuild = new();
    private readonly Ps3Button _btnRun = new();
    private readonly Ps3Button _btnCopy = new();
    private readonly Ps3Button _btnCancel = new();
    private readonly ComboBox _gfx = new();
    private readonly ComboBox _threads = new();
    private readonly TabControl _tabs = new();
    private readonly PictureBox _logo = new();
    private string? _projectDir;
    private CancellationTokenSource? _cts;
    private Process? _gameProc;
    private bool _building;
    private bool _buildCancelRequested;
    private bool _canLift, _canBuild, _canCopy, _canRun, _gfxUnlocked, _busy;
    private int _liftChunks;
    private Ps3Button? _btnRpcs3;
    private readonly Label _eta = new();
    private bool _controllerHiddenAfterBuild;

    private const int StallQuietMs = 4000;
    private const int MaxRunMs = 0;
    private static string RuntimeDir => Path.Combine(AppContext.BaseDirectory, "runtime");
    private static string CompilersDir => Path.Combine(AppContext.BaseDirectory, "Compilers-files");
    private static string ProjectsRoot => Path.Combine(AppContext.BaseDirectory, "projects");

    [DllImport("dwmapi.dll")]
    private static extern int DwmSetWindowAttribute(IntPtr hwnd, int attr, ref int value, int size);

    protected override CreateParams CreateParams
    {
        get
        {
            var cp = base.CreateParams;
            cp.ExStyle |= 0x02000000;
            return cp;
        }
    }

    public MainForm()
    {
        Text = "PPSX33  ·  PlayStation 3 Recompiler  ·  XXII";
        StartPosition = FormStartPosition.CenterScreen;
        Size = new Size(1280, 800);
        MinimumSize = new Size(1024, 640);
        BackColor = Bg;
        ForeColor = TextPri;
        Font = new Font("Segoe UI", 9.5f);
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

        var backdrop = new Panel { Dock = DockStyle.Fill, BackColor = Bg, Padding = new Padding(12) };
        var root = new TableLayoutPanel { Dock = DockStyle.Fill, ColumnCount = 2, RowCount = 1, BackColor = Bg };
        root.ColumnStyles.Add(new ColumnStyle(SizeType.Absolute, 280));
        root.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 100));

        var rail = new Panel { Dock = DockStyle.Fill, BackColor = Panel, Padding = new Padding(14) };
        var railLayout = new TableLayoutPanel { Dock = DockStyle.Fill, ColumnCount = 1, RowCount = 16, BackColor = Panel };
        for (int i = 0; i < 14; i++)
            railLayout.RowStyles.Add(new RowStyle(SizeType.AutoSize));

        _logo.Size = new Size(140, 140);
        _logo.SizeMode = PictureBoxSizeMode.Zoom;
        _logo.BackColor = Color.Transparent;
        _logo.Margin = new Padding(8, 4, 8, 12);
        try
        {
            // Form / taskbar icon (.ico)
            string[] icoCandidates = {
                Path.Combine(AppContext.BaseDirectory, "assets", "ppsx33-logo.ico"),
                Path.Combine(AppContext.BaseDirectory, "ppsx33-logo.ico"),
                Path.Combine(AppContext.BaseDirectory, "..", "assets", "ppsx33-logo.ico"),
                Path.Combine(AppContext.BaseDirectory, "..", "..", "assets", "ppsx33-logo.ico"),
            };
            foreach (var ico in icoCandidates)
            {
                if (!File.Exists(ico)) continue;
                Icon = new Icon(ico);
                break;
            }
            // Rail logo (.png)
            string[] pngCandidates = {
                Path.Combine(AppContext.BaseDirectory, "assets", "ppsx33-logo.png"),
                Path.Combine(AppContext.BaseDirectory, "ppsx33-logo.png"),
                Path.Combine(AppContext.BaseDirectory, "..", "assets", "ppsx33-logo.png"),
                Path.Combine(AppContext.BaseDirectory, "..", "..", "assets", "ppsx33-logo.png"),
            };
            foreach (var png in pngCandidates)
            {
                if (!File.Exists(png)) continue;
                _logo.Image = Image.FromFile(png);
                break;
            }
        }
        catch { /* missing assets */ }

        StyleBtn(_btnDecrypt, "Decrypt EBOOT", Panel2);
        StyleBtn(_btnLoad, "Load ELF", Panel2);
        StyleBtn(_btnLift, "Decompile", Panel2);
        StyleBtn(_btnBuild, "Build", Accent);
        StyleBtn(_btnRun, "Run", Panel2);
        StyleBtn(_btnCopy, "Copy to game folder", Panel2);
        StyleBtn(_btnCancel, "Cancel / Stop", Danger);

        _btnRpcs3 = new Ps3Button();
        StyleBtn(_btnRpcs3, "RPCS3 path…", Panel2);
        _btnRpcs3.Click += (_, _) => {
            using var ofd = new OpenFileDialog {
                Title = "Locate RPCS3.exe",
                Filter = "RPCS3|RPCS3.exe|Executables|*.exe|All|*.*"
            };
            if (ofd.ShowDialog(this) != DialogResult.OK) return;
            _settings.Rpcs3Path = ofd.FileName;
            SaveSettings();
            UpdateRpcs3Button();
            Append("RPCS3 path set: " + _settings.Rpcs3Path);
        };


        _gfx.DropDownStyle = ComboBoxStyle.DropDownList;
        _gfx.Items.AddRange(new object[] { "D3D11", "Vulkan (stub)", "OpenGL (stub)" });
        _gfx.SelectedIndex = 0;
        _gfx.Enabled = false;
        StyleCombo(_gfx);

        _threads.DropDownStyle = ComboBoxStyle.DropDownList;
        for (int i = 1; i <= Environment.ProcessorCount; i++) _threads.Items.Add(i.ToString());
        _threads.SelectedItem = Math.Min(4, Environment.ProcessorCount).ToString();
        StyleCombo(_threads);

        _eta.ForeColor = TextSec; _eta.AutoSize = true; _eta.Text = "";

        railLayout.Controls.Add(_logo, 0, 0);
        railLayout.Controls.Add(MakeLabel("WORKFLOW"), 0, 1);
        railLayout.Controls.Add(_btnDecrypt, 0, 2);
        railLayout.Controls.Add(_btnLoad, 0, 3);
        railLayout.Controls.Add(_btnLift, 0, 4);
        railLayout.Controls.Add(_btnBuild, 0, 5);
        railLayout.Controls.Add(_btnRun, 0, 6);
        railLayout.Controls.Add(_btnCopy, 0, 7);
        railLayout.Controls.Add(_btnCancel, 0, 8);
        railLayout.Controls.Add(_btnRpcs3, 0, 9);
        railLayout.Controls.Add(MakeLabel("GRAPHICS"), 0, 10);
        railLayout.Controls.Add(_gfx, 0, 11);
        railLayout.Controls.Add(MakeLabel("THREADS"), 0, 12);
        railLayout.Controls.Add(_threads, 0, 13);
        railLayout.Controls.Add(_eta, 0, 14);
        rail.Controls.Add(railLayout);

        var right = new TableLayoutPanel { Dock = DockStyle.Fill, ColumnCount = 1, RowCount = 3, BackColor = Bg };
        right.RowStyles.Add(new RowStyle(SizeType.Absolute, 36));
        right.RowStyles.Add(new RowStyle(SizeType.Percent, 100));
        right.RowStyles.Add(new RowStyle(SizeType.Absolute, 36));
        _project.Text = "PROJECT  /  NO PROJECT LOADED"; _project.ForeColor = TextPri; _project.BackColor = Color.FromArgb(12, 22, 47);
        _project.Dock = DockStyle.Fill; _project.TextAlign = ContentAlignment.MiddleLeft; _project.Padding = new Padding(12, 0, 0, 0);
        StyleTextBox(_log); StyleTextBox(_report); StyleTextBox(_roadmap);
        StyleTextBox(_requirements);
        _requirements.Text =
            "PPSX33 - Host requirements (research toolkit)\r\n" +
            "========================================\r\n\r\n" +
            "REQUIRED\r\n" +
            "  Disk type     HDD or SSD (SSD preferred for game data)\r\n" +
            "  RAM           8 GB system memory\r\n" +
            "  GPU           Any GPU with a WDDM driver (integrated OK)\r\n" +
            "  VRAM          Shared or dedicated; enough for a D3D11 swap chain\r\n" +
            "  Monitor       1280x720 or higher\r\n" +
            "  Graphics API  Direct3D 11 Feature Level 10_0+\r\n" +
            "  OS            Windows 10 or later (x64)\r\n\r\n" +
            "RECOMMENDED\r\n" +
            "  Disk type     SSD / NVMe\r\n" +
            "  RAM           16 GB or more\r\n" +
            "  GPU           Discrete GPU, D3D11 Feature Level 11_0\r\n" +
            "  VRAM          2 GB or more dedicated\r\n" +
            "  Monitor       1920x1080\r\n" +
            "  Graphics API  D3D11 FL11_0; future OpenGL 3.3/2.1 fallback\r\n\r\n" +
            "Commercial playability is not guaranteed at any host specification.\r\n";

        _roadmap.Text =
            "PPSX33 - Roadmap\r\n" +
            "========================================\r\n\r\n" +
            "PHASE 1 - XXI   (see roadmap/ folder for full detail)\r\n" +
            "  I-XIX     Core pipeline (TESTING)\r\n" +
            "  XX-A..G   Packaging & host UX (DONE)\r\n" +
            "  XX-H..O   Runtime base (DONE)\r\n" +
            "  XXI A-Z   Lettered post-XX work (DONE)\r\n\r\n" +
            "PHASE XXII - PIPELINE CORRECTNESS (ACTIVE)\r\n" +
            "  XXII-A   RSX present/color-buffer fidelity\r\n" +
            "  XXII-B   PPU soft-recovery / helper surface\r\n" +
            "  XXII-C+  SPU/MFC, LV2/HLE, modules, timing (planned)\r\n\r\n" +
            "FUTURE - OpenGL fallback\r\n" +
            "  Auto-select D3D11 when GPU supports it;\r\n" +
            "  otherwise OpenGL 3.3 (or 2.1) for no dedicated GPU.\r\n\r\n" +
            "Per-phase docs: roadmap/I.md ... roadmap/XXII.md\r\n" +
            "Requirements tab: minimum / recommended host specs.\r\n";

        _tabs.Dock = DockStyle.Fill; _tabs.DrawMode = TabDrawMode.OwnerDrawFixed; _tabs.ItemSize = new Size(110, 32); _tabs.SizeMode = TabSizeMode.Fixed; _tabs.DrawItem += DrawPs3Tab;
        AddTab("Console", _log); AddTab("Report", _report); AddTab("Roadmap", _roadmap); AddTab("Requirements", _requirements);
        BuildControllerTab();
        _status.TextAlign = ContentAlignment.MiddleLeft; _status.Padding = new Padding(8, 0, 0, 0);
        _status.Dock = DockStyle.Fill; _status.ForeColor = TextSec; _status.BackColor = Color.FromArgb(10, 17, 37);
        _progress.Dock = DockStyle.Bottom; _progress.Height = 6;

        right.Controls.Add(_project, 0, 0);
        right.Controls.Add(_tabs, 0, 1);
        var statusRow = new Panel { Dock = DockStyle.Fill, BackColor = Color.FromArgb(10, 17, 37) };
        statusRow.Controls.Add(_status);
        statusRow.Controls.Add(_progress);
        right.Controls.Add(statusRow, 0, 2);

        root.Controls.Add(rail, 0, 0);
        root.Controls.Add(right, 1, 0);
        backdrop.Controls.Add(root);
        Controls.Add(backdrop);

        _btnDecrypt.Click += async (_, _) => await DecryptEbootAndLoad();
        _btnLoad.Click += (_, _) => LoadElf(null);
        _btnLift.Click += async (_, _) => await RunStep("Decompiling", ct => Native.Lift(_projectDir!, out _), afterLift: true);
        _btnBuild.Click += async (_, _) => await BuildWithProgress();
        _btnRun.Click += async (_, _) => await RunGame();
        _btnCopy.Click += (_, _) => CopyToGameFolder();
        _btnCancel.Click += async (_, _) => {
            if (_building && !_buildCancelRequested) await ConfirmCancelBuildAsync();
            else StopGame();
        };

        Append("PPSX33 ready · core v1.0.0");
        Append("Settings: " + Settings.DataDir);
        UpdateButtons();
        Shown += (_, _) => CheckRpcs3OnStartup();
    }
}
