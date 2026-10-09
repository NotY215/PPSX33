using System.Text.Json;

namespace PS3Recomp.Gui;

/// <summary>User settings, stored as settings.json next to the exe. Phase 3-5 read these values.</summary>
public sealed class Settings
{
    public string GraphicsBackend { get; set; } = "D3D11";   // D3D10 | D3D11 | Vulkan  (Phase 5)
    public int PpuThreads { get; set; } = 4;                  // host threads used by the recompiled PPU/SPU runtime (Phase 3/4)
    public string Rpcs3Path { get; set; } = "";               // rpcs3.exe, used only to decrypt EBOOT.BIN

    private static string FilePath => Path.Combine(AppContext.BaseDirectory, "settings.json");

    public static Settings Load()
    {
        try { if (File.Exists(FilePath)) return JsonSerializer.Deserialize<Settings>(File.ReadAllText(FilePath)) ?? new Settings(); }
        catch { /* fall back to defaults */ }
        return new Settings();
    }

    public void Save() =>
        File.WriteAllText(FilePath, JsonSerializer.Serialize(this, new JsonSerializerOptions { WriteIndented = true }));
}
