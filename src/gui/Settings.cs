using System.Text.Json;

namespace PS3Recomp.Gui;

/// <summary>User settings under %AppData%/PPSX33/settings.json.</summary>
public sealed class Settings
{
    public string GraphicsBackend { get; set; } = "D3D11";
    public int PpuThreads { get; set; } = 4;
    public string Rpcs3Path { get; set; } = "";
    public string LastElfPath { get; set; } = "";
    public string LastBinPath { get; set; } = "";

    public static string DataDir =>
        Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData), "PPSX33");

    private static string FilePath => Path.Combine(DataDir, "settings.json");

    public static Settings Load()
    {
        try
        {
            Directory.CreateDirectory(DataDir);
            if (File.Exists(FilePath))
                return JsonSerializer.Deserialize<Settings>(File.ReadAllText(FilePath)) ?? new Settings();
        }
        catch { }
        return new Settings();
    }

    public void Save()
    {
        try
        {
            Directory.CreateDirectory(DataDir);
            File.WriteAllText(FilePath, JsonSerializer.Serialize(this, new JsonSerializerOptions { WriteIndented = true }));
        }
        catch { }
    }
}
