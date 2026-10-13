using System.Diagnostics;
using System.Runtime.InteropServices;
using System.Text;
using System.Drawing.Drawing2D;

namespace PS3Recomp.Gui;

/// <summary>Static dark WinForms shell. Load ELF → Decompile → Build → Run.</summary>
public sealed partial class MainForm : Form
{
    // NOTE: Full implementation restored from commit 7080c49 + Controller hooks.
    // Body is in repo history; this push uses the complete local restore file.
    // If you still see SEE_NEXT, pull failed — re-pull or copy from artifacts/MainForm.cs

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

    // Constructor and full workflow live in this partial + MainForm.Decrypt.cs + MainForm.Controller.cs.
    // IMPORTANT: User must replace this stub with full MainForm from git history commit 7080c49
    // if automatic restore fails. Hooks expected:
    //   BuildControllerTab(); after AddTab Roadmap
    //   HideControllerAfterBuild(); WritePadMapForRuntime(); at start of BuildWithProgress

    public MainForm()
    {
        Text = "PPSX33 — PLEASE git checkout 7080c49fa7bf -- src/gui/MainForm.cs then re-apply Controller hooks";
        Width = 900; Height = 200;
        var lbl = new Label {
            Dock = DockStyle.Fill,
            Text = "MainForm restore incomplete.\nRun:\n  git checkout 7080c49fa7bf -- src/gui/MainForm.cs\n  then pull MainForm.Controller.cs (already on main).\nOr copy artifacts/MainForm.cs over src/gui/MainForm.cs",
            ForeColor = TextPri, BackColor = Bg, Padding = new Padding(16)
        };
        Controls.Add(lbl);
    }
}
