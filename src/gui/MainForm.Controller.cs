using System.Runtime.InteropServices;
using System.Text.Json;

namespace PS3Recomp.Gui;

public sealed partial class MainForm
{
    private Panel? _controllerPanel;
    private TabPage? _controllerTab;
    private IntPtr _nativeController;
    private NativeControllerMapCallback? _nativeControllerCallback;

    [UnmanagedFunctionPointer(CallingConvention.StdCall, CharSet = CharSet.Unicode)]
    private delegate void NativeControllerMapCallback([MarshalAs(UnmanagedType.LPWStr)] string json);

    private static class NativeController
    {
        [DllImport("PPSX33ControllerNative.dll", CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode, ExactSpelling = true)]
        internal static extern IntPtr PPSX33_ControllerCreate(
            IntPtr parent, NativeControllerMapCallback callback,
            [MarshalAs(UnmanagedType.LPWStr)] string initialJson);

        [DllImport("PPSX33ControllerNative.dll", CallingConvention = CallingConvention.StdCall, ExactSpelling = true)]
        internal static extern void PPSX33_ControllerResize(IntPtr window, int width, int height);

        [DllImport("PPSX33ControllerNative.dll", CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode, ExactSpelling = true)]
        internal static extern void PPSX33_ControllerSetMap([MarshalAs(UnmanagedType.LPWStr)] string json);

        [DllImport("PPSX33ControllerNative.dll", CallingConvention = CallingConvention.StdCall)]
        internal static extern void PPSX33_ControllerDestroy(IntPtr window);
    }

    void BuildControllerTab()
    {
        if (_controllerTab != null) return;

        _controllerPanel = new Panel
        {
            Dock = DockStyle.Fill,
            BackColor = Color.FromArgb(7, 13, 24),
            Padding = Padding.Empty
        };
        _controllerPanel.HandleCreated += (_, _) => InitializeNativeController();
        _controllerPanel.Resize += (_, _) =>
        {
            if (_nativeController != IntPtr.Zero && _controllerPanel != null)
                NativeController.PPSX33_ControllerResize(
                    _nativeController, _controllerPanel.ClientSize.Width, _controllerPanel.ClientSize.Height);
        };

        _controllerTab = new TabPage("Controller")
        {
            BackColor = Color.FromArgb(7, 13, 24),
            Padding = Padding.Empty
        };
        _controllerTab.Controls.Add(_controllerPanel);
        _tabs.TabPages.Add(_controllerTab);
    }

    private void InitializeNativeController()
    {
        if (_nativeController != IntPtr.Zero || _controllerPanel == null || _controllerPanel.IsDisposed)
            return;

        try
        {
            _nativeControllerCallback = OnNativeControllerMapSaved;
            string initialMap = JsonSerializer.Serialize(_settings.KeyMap ?? KeyMap.Defaults());
            _nativeController = NativeController.PPSX33_ControllerCreate(
                _controllerPanel.Handle, _nativeControllerCallback, initialMap);
            if (_nativeController == IntPtr.Zero)
            {
                Append("Native controller studio failed to create its child window.");
                return;
            }

            NativeController.PPSX33_ControllerResize(
                _nativeController, _controllerPanel.ClientSize.Width, _controllerPanel.ClientSize.Height);
            Append("Native C++ Controller Studio ready.");
        }
        catch (DllNotFoundException ex)
        {
            Append("Native controller library missing: " + ex.Message);
        }
        catch (EntryPointNotFoundException ex)
        {
            Append("Native controller library exports are incomplete: " + ex.Message);
        }
        catch (Exception ex)
        {
            Append("Native controller initialization failed: " + ex.Message);
        }
    }

    private void OnNativeControllerMapSaved(string json)
    {
        try
        {
            var imported = JsonSerializer.Deserialize<KeyMap>(json);
            if (imported == null) return;

            _settings.KeyMap = imported;
            SaveSettings();
            WritePadMapForRuntime();
            _controllerMapped = true;
            Append("Controller mapping saved from native C++ Controller Studio.");
            UpdateButtons();
        }
        catch (Exception ex)
        {
            Append("Controller mapping could not be saved: " + ex.Message);
        }
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
        catch (Exception ex)
        {
            Append("pad_map write failed: " + ex.Message);
        }
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

    private void DisposeNativeController()
    {
        if (_nativeController == IntPtr.Zero) return;
        try { NativeController.PPSX33_ControllerDestroy(_nativeController); } catch { }
        _nativeController = IntPtr.Zero;
        _nativeControllerCallback = null;
    }
}
