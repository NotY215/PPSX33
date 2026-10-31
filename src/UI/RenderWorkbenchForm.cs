using System.Diagnostics;
using System.Drawing.Drawing2D;
using System.Text;

namespace PS3Recomp.Gui;

/// <summary>
/// PPSX33.exe -render — rebuild selected runtime DLLs (not full game EXE / PPU lift).
/// Dark theme matches MainForm.
/// </summary>
public sealed class RenderWorkbenchForm : Form
{
    static readonly Color Bg = Color.FromArgb(0x09, 0x0D, 0x14);
    static readonly Color Panel = Color.FromArgb(0x0D, 0x11, 0x17);
    static readonly Color Panel2 = Color.FromArgb(0x16, 0x1B, 0x22);
    static readonly Color BorderC = Color.FromArgb(0x30, 0x36, 0x3D);
    static readonly Color TextPri = Color.FromArgb(0xE6, 0xED, 0xF3);
    static readonly Color TextSec = Color.FromArgb(0x8B, 0x94, 0x9E);
    static readonly Color Accent = Color.FromArgb(0x38, 0xBD, 0xF8);
    static readonly Color AccentBlue = Color.FromArgb(0x1F, 0x6F, 0xEB);

    readonly TextBox _log = new();
    readonly CheckedListBox _dlls = new();
    readonly Button _btnBuild = new();
    readonly Button _btnAll = new();
    readonly Button _btnNone = new();
    readonly Label _hint = new();
    readonly Panel _welcome = new();
    readonly Label _welcomeTitle = new();
    readonly Label _welcomeSub = new();
    readonly System.Windows.Forms.Timer _welcomeTimer = new();
    int _welcomeAlpha = 255;

    static readonly (string Id, string Title, string Note)[] Modules =
    {
        ("resolution", "resolution.dll", "Host output size (576p) + pitch helpers"),
        ("api", "api.dll (Vulkan)", "Vulkan 1.2 instance/device/swapchain/present"),
        ("rsx", "rsx.dll", "FIFO / GCM host state (untranslated RSX stream)"),
        ("render", "render.dll", "Draw batch + present queue + RSX→host translation"),
        ("ps3rt", "ps3rt.dll", "HLE + SPU + audio + loads the modules above"),
    };

    public RenderWorkbenchForm()
    {
        Text = "PPSX33 — Render module workbench (-render)";
        Width = 1000;
        Height = 680;
        StartPosition = FormStartPosition.CenterScreen;
        BackColor = Bg;
        ForeColor = TextPri;
        Font = new Font("Segoe UI", 9.5f);
        TrySetIcon();

        // --- left: modules ---
        _hint.Text = "Rebuild only graphics/runtime DLLs. Game EXE (PPU lift) is not rebuilt here.\r\n"
                   + "Tick modules, then Build. Requires an existing CMake build/ tree.";
        _hint.ForeColor = TextSec;
        _hint.BackColor = Panel;
        _hint.Padding = new Padding(10, 8, 10, 8);
        _hint.Height = 52;
        _hint.Dock = DockStyle.Top;

        _dlls.BackColor = Panel2;
        _dlls.ForeColor = TextPri;
        _dlls.BorderStyle = BorderStyle.None;
        _dlls.CheckOnClick = true;
        _dlls.Font = new Font("Segoe UI", 10f);
        _dlls.IntegralHeight = false;
        _dlls.Dock = DockStyle.Fill;
        foreach (var m in Modules)
            _dlls.Items.Add($"{m.Title}  —  {m.Note}", m.Id is "resolution" or "api" or "rsx" or "render");

        StyleBtn(_btnAll, "Select all", Panel2);
        StyleBtn(_btnNone, "Select none", Panel2);
        StyleBtn(_btnBuild, "Build selected DLLs", AccentBlue);
        var btnLoad = new Button();
        var btnDecomp = new Button();
        StyleBtn(btnLoad, "Load ELF…", Panel2);
        StyleBtn(btnDecomp, "Decompile (note)", Panel2);
        btnLoad.Width = 110;
        btnDecomp.Width = 130;
        btnLoad.Click += (_, _) =>
        {
            using var ofd = new OpenFileDialog
            {
                Filter = "Decrypted ELF|*.elf;*.bin|All|*.*",
                Title = "Load decrypted ELF (for module rebuild context)"
            };
            if (ofd.ShowDialog(this) == DialogResult.OK)
            {
                Log("ELF: " + ofd.FileName);
                Log("Note: -render does not re-lift PPU; use main UI for full decompile.");
            }
        };
        btnDecomp.Click += (_, _) =>
        {
            Log("Decompile in -render: use main PPSX33 UI for PPU/SPU lift.");
            Log("Here you only rebuild runtime DLLs (api/rsx/render/resolution/ps3rt).");
        };
        _btnAll.Click += (_, _) => { for (int i = 0; i < _dlls.Items.Count; i++) _dlls.SetItemChecked(i, true); };
        _btnNone.Click += (_, _) => { for (int i = 0; i < _dlls.Items.Count; i++) _dlls.SetItemChecked(i, false); };
        _btnBuild.Click += async (_, _) => await BuildSelectedAsync();

        var btnRow = new FlowLayoutPanel
        {
            Dock = DockStyle.Bottom,
            Height = 88,
            BackColor = Panel,
            Padding = new Padding(8, 8, 8, 8),
            FlowDirection = FlowDirection.LeftToRight
        };
        btnRow.Controls.Add(_btnAll);
        btnRow.Controls.Add(_btnNone);
        btnRow.Controls.Add(_btnBuild);
        btnRow.Controls.Add(btnLoad);
        btnRow.Controls.Add(btnDecomp);

        var left = new Panel { Dock = DockStyle.Fill, BackColor = Panel, Padding = new Padding(4) };
        left.Controls.Add(_dlls);
        left.Controls.Add(btnRow);
        left.Controls.Add(_hint);

        // --- right: console ---
        _log.Multiline = true;
        _log.ScrollBars = ScrollBars.Both;
        _log.ReadOnly = true;
        _log.Dock = DockStyle.Fill;
        _log.BorderStyle = BorderStyle.None;
        _log.BackColor = Color.FromArgb(0x0A, 0x0E, 0x14);
        _log.ForeColor = Color.FromArgb(0xC9, 0xD1, 0xD9);
        _log.Font = new Font("Consolas", 9.5f);

        var split = new SplitContainer
        {
            Dock = DockStyle.Fill,
            Orientation = Orientation.Vertical,
            BackColor = BorderC,
            SplitterWidth = 3
        };
        split.Panel1.BackColor = Panel;
        split.Panel2.BackColor = Bg;
        split.Panel1.Controls.Add(left);
        split.Panel2.Controls.Add(_log);

        var tabs = new TabControl { Dock = DockStyle.Fill, BackColor = Panel };
        var tabDll = new TabPage("DLL modules") { BackColor = Panel, ForeColor = TextPri };
        tabDll.Controls.Add(split);
        var tabAbout = new TabPage("About") { BackColor = Panel, ForeColor = TextPri };
        var about = new TextBox
        {
            Dock = DockStyle.Fill,
            Multiline = true,
            ReadOnly = true,
            BorderStyle = BorderStyle.None,
            BackColor = Panel2,
            ForeColor = TextPri,
            Font = new Font("Segoe UI", 10f),
            Text =
                "Architecture\r\n\r\n" +
                "  Game EXE        — PPU lift only (rebuild when PPU/SPU lift changes)\r\n" +
                "  ps3rt.dll       — HLE kernel, SPU, audio\r\n" +
                "  resolution.dll  — 720×576 host present helpers\r\n" +
                "  api / rsx / render — still linked inside ps3rt this milestone;\r\n" +
                "                      -render rebuilds ps3rt for those targets\r\n" +
                "  vulkan-1.dll    — system Vulkan loader (copied beside EXE)\r\n\r\n" +
                "Main UI:  PPSX33.exe\r\n" +
                "Workbench: PPSX33.exe -render\r\n\r\n" +
                "Copy to Game Folder also places resolution.dll next to EBOOT when present."
        };
        tabAbout.Controls.Add(about);
        tabs.TabPages.Add(tabDll);
        tabs.TabPages.Add(tabAbout);
        Controls.Add(tabs);

        // Welcome overlay
        _welcome.Dock = DockStyle.Fill;
        _welcome.BackColor = Color.FromArgb(255, Bg);
        _welcomeTitle.Text = "Welcome to Private mode";
        _welcomeTitle.Font = new Font("Segoe UI Semibold", 22f);
        _welcomeTitle.ForeColor = Accent;
        _welcomeTitle.AutoSize = true;
        _welcomeSub.Text = "PPSX33 render workbench — rebuild graphics DLLs without re-lifting the game.";
        _welcomeSub.Font = new Font("Segoe UI", 11f);
        _welcomeSub.ForeColor = TextSec;
        _welcomeSub.AutoSize = true;
        _welcome.Controls.Add(_welcomeTitle);
        _welcome.Controls.Add(_welcomeSub);
        _welcome.Resize += (_, _) => LayoutWelcome();
        Controls.Add(_welcome);
        _welcome.BringToFront();
        LayoutWelcome();

        _welcomeTimer.Interval = 40;
        _welcomeTimer.Tick += (_, _) =>
        {
            _welcomeAlpha -= 12;
            if (_welcomeAlpha <= 0)
            {
                _welcomeTimer.Stop();
                _welcome.Visible = false;
                Controls.Remove(_welcome);
                _welcome.Dispose();
                return;
            }
            _welcome.BackColor = Color.FromArgb(Math.Clamp(_welcomeAlpha, 0, 255), Bg);
            _welcomeTitle.ForeColor = Color.FromArgb(Math.Clamp(_welcomeAlpha, 0, 255), Accent);
            _welcomeSub.ForeColor = Color.FromArgb(Math.Clamp(_welcomeAlpha, 0, 255), TextSec);
        };
        Shown += (_, _) =>
        {
            split.SplitterDistance = Math.Max(380, (int)(ClientSize.Width * 0.48));
            _welcomeTimer.Start();
        };

        Log("PPSX33 render workbench ready. Select DLLs and Build.");
    }

    void LayoutWelcome()
    {
        if (!_welcome.IsHandleCreated) return;
        _welcomeTitle.Location = new Point(
            Math.Max(24, (_welcome.ClientSize.Width - _welcomeTitle.Width) / 2),
            Math.Max(40, _welcome.ClientSize.Height / 2 - 40));
        _welcomeSub.Location = new Point(
            Math.Max(24, (_welcome.ClientSize.Width - _welcomeSub.Width) / 2),
            _welcomeTitle.Bottom + 12);
    }

    void TrySetIcon()
    {
        try
        {
            foreach (var p in new[]
            {
                Path.Combine(AppContext.BaseDirectory, "ppsx33-logo.ico"),
                Path.Combine(AppContext.BaseDirectory, "assets", "ppsx33-logo.ico"),
            })
            {
                if (File.Exists(p)) { Icon = new Icon(p); break; }
            }
        }
        catch { }
    }

    static void StyleBtn(Button b, string text, Color back)
    {
        b.Text = text;
        b.FlatStyle = FlatStyle.Flat;
        b.FlatAppearance.BorderColor = BorderC;
        b.FlatAppearance.BorderSize = 1;
        b.BackColor = back;
        b.ForeColor = TextPri;
        b.Height = 34;
        b.Width = text.Length > 14 ? 160 : 110;
        b.Cursor = Cursors.Hand;
        b.Margin = new Padding(4, 2, 4, 2);
    }

    void Log(string s)
    {
        if (IsDisposed) return;
        if (InvokeRequired) { BeginInvoke(() => Log(s)); return; }
        _log.AppendText(s.TrimEnd() + Environment.NewLine);
    }

    async Task BuildSelectedAsync()
    {
        var selected = new List<string>();
        for (int i = 0; i < _dlls.Items.Count; i++)
            if (_dlls.GetItemChecked(i)) selected.Add(Modules[i].Id);

        if (selected.Count == 0)
        {
            Log("No modules selected.");
            return;
        }

        _btnBuild.Enabled = false;
        Log("Building: " + string.Join(", ", selected));
        string root = FindRepoRoot();
        string buildDir = Path.Combine(root, "build");
        if (!Directory.Exists(buildDir))
        {
            Log("No build/ directory under: " + root);
            Log("Configure the CMake project once from Visual Studio / main build first.");
            _btnBuild.Enabled = true;
            return;
        }

        var targets = selected.Select(id => id switch
        {
            "resolution" => "ppsx33_resolution",
            "api" => "ppsx33_api",
            "rsx" => "ppsx33_rsx",
            "render" => "ppsx33_render",
            "ps3rt" => "ps3rt",
            _ => id
        }).ToArray();

        string? cmake = FindCMake();
        if (string.IsNullOrEmpty(cmake))
        {
            Log("cmake.exe not found on PATH or VS install.");
            _btnBuild.Enabled = true;
            return;
        }

        var psi = new ProcessStartInfo
        {
            FileName = cmake,
            Arguments = $"--build \"{buildDir}\" --config Release --target {string.Join(" --target ", targets)}",
            WorkingDirectory = buildDir,
            RedirectStandardOutput = true,
            RedirectStandardError = true,
            UseShellExecute = false,
            CreateNoWindow = true
        };
        try
        {
            using var p = Process.Start(psi);
            if (p == null)
            {
                Log("Failed to start cmake.");
                _btnBuild.Enabled = true;
                return;
            }
            p.OutputDataReceived += (_, e) => { if (e.Data != null) Log(e.Data); };
            p.ErrorDataReceived += (_, e) => { if (e.Data != null) Log(e.Data); };
            p.BeginOutputReadLine();
            p.BeginErrorReadLine();
            await p.WaitForExitAsync();
            Log(p.ExitCode == 0 ? "Build finished OK. Check build/dist for DLLs." : $"Build failed (exit {p.ExitCode}).");
        }
        catch (Exception ex)
        {
            Log(ex.Message);
        }
        _btnBuild.Enabled = true;
    }

    static string FindRepoRoot()
    {
        string d = AppContext.BaseDirectory;
        for (int i = 0; i < 8; i++)
        {
            if (File.Exists(Path.Combine(d, "CMakeLists.txt"))) return d;
            string parent = Path.GetFullPath(Path.Combine(d, ".."));
            if (parent == d) break;
            d = parent;
        }
        return AppContext.BaseDirectory;
    }

    static string? FindCMake()
    {
        string? pathEnv = Environment.GetEnvironmentVariable("PATH");
        if (!string.IsNullOrEmpty(pathEnv))
        {
            foreach (var p in pathEnv.Split(Path.PathSeparator))
            {
                if (string.IsNullOrWhiteSpace(p)) continue;
                var c = Path.Combine(p.Trim(), "cmake.exe");
                if (File.Exists(c)) return c;
            }
        }
        string[] hints =
        {
            @"C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe",
            @"C:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe",
            @"C:\Program Files\CMake\bin\cmake.exe",
        };
        foreach (var h in hints)
            if (File.Exists(h)) return h;
        return null;
    }
}
