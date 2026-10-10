using System.Text.Json;

namespace PS3Recomp.Gui;

/// <summary>
/// Persisted under %APPDATA%/PPSX33/settings.json
/// Holds RPCS3 path, last ELF/BIN, graphics, threads, and DualShock key map.
/// </summary>
public sealed class Settings
{
    public string GraphicsBackend { get; set; } = "D3D11";
    public int PpuThreads { get; set; } = Math.Clamp(Environment.ProcessorCount, 1, 16);
    public string Rpcs3Path { get; set; } = "";
    public string LastElfPath { get; set; } = "";
    public string LastBinPath { get; set; } = "";
    public KeyMap KeyMap { get; set; } = KeyMap.Defaults();

    public static string DataDir
    {
        get
        {
            string baseDir = Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData);
            if (string.IsNullOrWhiteSpace(baseDir))
                baseDir = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.UserProfile), "AppData", "Roaming");
            string dir = Path.Combine(baseDir, "PPSX33");
            try { Directory.CreateDirectory(dir); } catch { /* ignore */ }
            return dir;
        }
    }

    static string FilePath => Path.Combine(DataDir, "settings.json");

    static readonly JsonSerializerOptions JsonOpts = new()
    {
        WriteIndented = true,
        PropertyNameCaseInsensitive = true
    };

    public static Settings Load()
    {
        try
        {
            if (File.Exists(FilePath))
            {
                string json = File.ReadAllText(FilePath);
                var s = JsonSerializer.Deserialize<Settings>(json, JsonOpts);
                if (s != null)
                {
                    if (s.KeyMap == null) s.KeyMap = KeyMap.Defaults();
                    return s;
                }
            }
        }
        catch { /* fall through to defaults */ }
        return new Settings();
    }

    public void Save()
    {
        try
        {
            if (KeyMap == null) KeyMap = KeyMap.Defaults();
            Directory.CreateDirectory(DataDir);
            File.WriteAllText(FilePath, JsonSerializer.Serialize(this, JsonOpts));
        }
        catch { /* ignore IO errors */ }
    }
}
