using System.Text.Json;

namespace PS3Recomp.Gui;

/// <summary>
/// Persisted under %APPDATA%/PPSX33/settings.json
/// Paths, RPCS3, graphics, threads, DualShock key map, terms acceptance.
/// </summary>
public sealed class Settings
{
    public string GraphicsBackend { get; set; } = "Vulkan";
    public int PpuThreads { get; set; } = Math.Clamp(Environment.ProcessorCount, 1, 16);
    public string Rpcs3Path { get; set; } = "";
    /// <summary>Last decrypted/loaded ELF path.</summary>
    public string LastElfPath { get; set; } = "";
    /// <summary>Last EBOOT.BIN / encrypted binary path.</summary>
    public string LastBinPath { get; set; } = "";
    /// <summary>Parent of the game folder last chosen (e.g. E:\Games).</summary>
    public string LastGamesRootPath { get; set; } = "";
    /// <summary>Full game folder containing EBOOT.BIN (e.g. E:\Games\GOW3).</summary>
    public string LastGameFolderPath { get; set; } = "";
    /// <summary>Resolved EBOOT.BIN full path inside the game folder.</summary>
    public string LastEbootPath { get; set; } = "";
    /// <summary>Last project output folder (…/projects/Name/output).</summary>
    public string LastOutputFolderPath { get; set; } = "";
    public KeyMap KeyMap { get; set; } = KeyMap.Defaults();
    public bool TermsAccepted { get; set; } = false;
    public string TermsAcceptedBy { get; set; } = "";
    public string TermsVersion { get; set; } = "2026-10-14";

    public static string DataDir
    {
        get
        {
            string baseDir = Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData);
            if (string.IsNullOrWhiteSpace(baseDir))
                baseDir = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.UserProfile), "AppData", "Roaming");
            string dir = Path.Combine(baseDir, "PPSX33");
            try { Directory.CreateDirectory(dir); } catch { }
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
        catch { }
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
        catch { }
    }
}
