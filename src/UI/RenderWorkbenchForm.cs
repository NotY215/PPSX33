using System.Diagnostics;
using System.Text;

namespace PS3Recomp.Gui;

/// <summary>
/// PPSX33.exe -render — rebuild only selected runtime DLLs (not full game EXE / PPU lift).
/// </summary>
public sealed class RenderWorkbenchForm : Form
{
    readonly TextBox _log = new() { Multiline = true, ScrollBars = ScrollBars.Both, ReadOnly = true, Dock = DockStyle.Fill, Font = new Font("Consolas", 9f) };
    readonly CheckedListBox _dlls = new() { Dock = DockStyle.Fill, CheckOnClick = true };
    readonly Button _btnBuild = new() { Text = "Build selected DLLs", Height = 36 };
    readonly Button _btnAll = new() { Text = "Select all", Height = 28 };
    readonly Button _btnNone = new() { Text = "Select none", Height = 28 };
    readonly Label _hint = new() { Dock = DockStyle.Top, Height = 48, AutoSize = false };

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
        Width = 960;
        Height = 640;
        StartPosition = FormStartPosition.CenterScreen;
        BackColor = Color.FromArgb(13, 17, 23);
        ForeColor = Color.FromArgb(230, 237, 243);

        _hint.ForeColor = Color.FromArgb(201, 209, 217);
        _hint.Text = "Rebuild only graphics/runtime DLLs. Game EXE (PPU lift) is not rebuilt here.\r\n"
            + "Tick modules to regenerate, then Build. Requires CMake build tree.";

        foreach (var m in Modules)
            _dlls.Items.Add($"{m.Title}  —  {m.Note}", m.Id is "resolution" or "api" or "rsx" or "render");

        var left = new Panel { Dock = DockStyle.Left, Width = 420, Padding = new Padding(8) };
        left.Controls.Add(_dlls);
        var btns = new FlowLayoutPanel { Dock = DockStyle.Bottom, Height = 80, FlowDirection = FlowDirection.LeftToRight };
        _btnAll.Click += (_, _) => { for (int i = 0; i < _dlls.Items.Count; i++) _dlls.SetItemChecked(i, true); };
        _btnNone.Click += (_, _) => { for (int i = 0; i < _dlls.Items.Count; i++) _dlls.SetItemChecked(i, false); };
        _btnBuild.Click += async (_, _) => await BuildSelectedAsync();
        btns.Controls.Add(_btnAll);
        btns.Controls.Add(_btnNone);
        btns.Controls.Add(_btnBuild);
        left.Controls.Add(btns);
        left.Controls.Add(_hint);

        var tabs = new TabControl { Dock = DockStyle.Fill };
        var tabDll = new TabPage("DLL modules");
        tabDll.Controls.Add(left);
        var tabConsole = new TabPage("Console");
        _log.BackColor = Color.FromArgb(13, 17, 23);
        _log.ForeColor = Color.FromArgb(201, 209, 217);
        tabConsole.Controls.Add(_log);
        tabs.TabPages.Add(tabDll);
        tabs.TabPages.Add(tabConsole);
        // Put dll list full in tab
        tabDll.Controls.Clear();
        var split = new SplitContainer { Dock = DockStyle.Fill, Orientation = Orientation.Vertical, SplitterDistance = 420 };
        split.Panel1.Controls.Add(_hint);
        split.Panel1.Controls.Add(_dlls);
        split.Panel1.Controls.Add(btns);
        _dlls.Dock = DockStyle.Fill;
        btns.Dock = DockStyle.Bottom;
        _hint.Dock = DockStyle.Top;
        split.Panel2.Controls.Add(_log);
        tabDll.Controls.Add(split);
        tabs.TabPages.Clear();
        tabs.TabPages.Add(tabDll);
        var tabHelp = new TabPage("About");
        tabHelp.Controls.Add(new Label
        {
            Dock = DockStyle.Fill,
            Padding = new Padding(12),
            ForeColor = Color.FromArgb(201, 209, 217),
            Text = "Architecture\r\n\r\n"
                + "  Game EXE     — PPU lift only (rebuild when PPU/SPU lift changes)\r\n"
                + "  ps3rt.dll    — HLE kernel, SPU, audio; loads graphics modules\r\n"
                + "  rsx.dll      — FIFO / surfaces / textures (guest RSX stream)\r\n"
                + "  render.dll   — RSX→host translation, present queue\r\n"
                + "  api.dll      — Vulkan 1.2 (OpenGL reserved, not selectable yet)\r\n"
                + "  resolution.dll — 720×576 host present\r\n\r\n"
                + "Main UI: PPSX33.exe\r\nRender workbench: PPSX33.exe -render\r\n"
        });
        tabs.TabPages.Add(tabHelp);
        Controls.Add(tabs);
        Log("PPSX33 render workbench ready. Select DLLs and Build.");
    }

    void Log(string s)
    {
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
            Log("No build/ directory. Configure the CMake project once from the main UI or IDE.");
            _btnBuild.Enabled = true;
            return;
        }

        // Map module id → CMake target name
        var targets = selected.Select(id => id switch
        {
            "resolution" => "ppsx33_resolution",
            "api" => "ppsx33_api",
            "rsx" => "ppsx33_rsx",
            "render" => "ppsx33_render",
            "ps3rt" => "ps3rt",
            _ => id
        }).ToArray();

        string cmake = FindCMake();
        if (cmake == null)
        {
            Log("cmake.exe not found on PATH.");
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
            using var p = Process.Start(psi)!;
            var sb = new StringBuilder();
            p.OutputDataReceived += (_, e) => { if (e.Data != null) Log(e.Data); };
            p.ErrorDataReceived += (_, e) => { if (e.Data != null) Log(e.Data); };
            p.BeginOutputReadLine();
            p.BeginErrorReadLine();
            await p.WaitForExitAsync();
            Log(p.ExitCode == 0 ? "Build finished OK." : $"Build failed (exit {p.ExitCode}).");
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
        for (int i = 0; i < 6; i++)
        {
            if (File.Exists(Path.Combine(d, "CMakeLists.txt"))) return d;
            d = Path.GetFullPath(Path.Combine(d, ".."));
        }
        return AppContext.BaseDirectory;
    }

    static string? FindCMake()
    {
        foreach (var p in Environment.GetEnvironmentVariable("PATH")!.Split(Path.PathSeparator))
        {
            var c = Path.Combine(p, "cmake.exe");
            if (File.Exists(c)) return c;
        }
        string[] hints =
        {
            @"C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe",
            @"C:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe",
        };
        return hints.FirstOrDefault(File.Exists);
    }
}
