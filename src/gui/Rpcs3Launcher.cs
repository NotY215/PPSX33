using System.Diagnostics;

namespace PS3Recomp.Gui;

/// <summary>
/// RPCS3 is only used to obtain a decrypted ELF. This app does not play games inside RPCS3.
/// Also locates decrypted ELFs under common RPCS3 cache paths and next to EBOOT.BIN.
/// </summary>
internal static class Rpcs3Launcher
{
    public static bool Launch(string rpcs3Exe, string ebootPath, out string error)
    {
        error = "";
        try
        {
            if (!File.Exists(rpcs3Exe)) { error = "rpcs3.exe not found. Set the path when prompted."; return false; }
            Process.Start(new ProcessStartInfo(rpcs3Exe, $"\"{ebootPath}\"") { UseShellExecute = false });
            return true;
        }
        catch (Exception ex) { error = ex.Message; return false; }
    }

    /// <summary>Search for likely decrypted ELF files. Newest first.</summary>
    public static List<string> FindDecryptedElfs(string? rpcs3Exe, string? ebootPath)
    {
        var roots = new List<string>();
        if (!string.IsNullOrEmpty(ebootPath))
        {
            var dir = Path.GetDirectoryName(ebootPath);
            if (!string.IsNullOrEmpty(dir)) roots.Add(dir);
        }
        if (!string.IsNullOrEmpty(rpcs3Exe))
        {
            var rpcs3Dir = Path.GetDirectoryName(rpcs3Exe);
            if (!string.IsNullOrEmpty(rpcs3Dir))
            {
                roots.Add(Path.Combine(rpcs3Dir, "cache"));
                roots.Add(Path.Combine(rpcs3Dir, "dev_hdd0", "game"));
                roots.Add(Path.Combine(rpcs3Dir, "dev_flash"));
            }
        }
        string appData = Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData);
        roots.Add(Path.Combine(appData, "rpcs3", "cache"));
        roots.Add(Path.Combine(appData, "rpcs3", "dev_hdd0", "game"));
        string docs = Environment.GetFolderPath(Environment.SpecialFolder.MyDocuments);
        roots.Add(Path.Combine(docs, "RPCS3", "cache"));

        var hits = new List<(string path, DateTime mtime)>();
        foreach (var root in roots.Distinct(StringComparer.OrdinalIgnoreCase))
        {
            if (!Directory.Exists(root)) continue;
            try
            {
                // Prefer shallow then one level deep for speed
                ScanDir(root, hits, depth: 0, maxDepth: 3);
            }
            catch { /* ignore inaccessible trees */ }
        }

        return hits
            .OrderByDescending(h => h.mtime)
            .Select(h => h.path)
            .Distinct(StringComparer.OrdinalIgnoreCase)
            .Take(40)
            .ToList();
    }

    private static void ScanDir(string dir, List<(string path, DateTime mtime)> hits, int depth, int maxDepth)
    {
        if (depth > maxDepth) return;
        try
        {
            foreach (var file in Directory.EnumerateFiles(dir))
            {
                string name = Path.GetFileName(file);
                string ext = Path.GetExtension(file).ToLowerInvariant();
                bool nameHit =
                    name.Equals("EBOOT.elf", StringComparison.OrdinalIgnoreCase) ||
                    name.EndsWith(".elf", StringComparison.OrdinalIgnoreCase) ||
                    (name.StartsWith("EBOOT", StringComparison.OrdinalIgnoreCase) && ext != ".bin");
                if (!nameHit) continue;
                if (!LooksLikeElf(file)) continue;
                hits.Add((file, File.GetLastWriteTimeUtc(file)));
            }
            if (depth == maxDepth) return;
            foreach (var sub in Directory.EnumerateDirectories(dir))
            {
                string leaf = Path.GetFileName(sub);
                // Skip huge or irrelevant trees
                if (leaf.Equals("shaderlog", StringComparison.OrdinalIgnoreCase)) continue;
                if (leaf.Equals("captures", StringComparison.OrdinalIgnoreCase)) continue;
                ScanDir(sub, hits, depth + 1, maxDepth);
            }
        }
        catch { /* skip */ }
    }

    private static bool LooksLikeElf(string path)
    {
        try
        {
            var fi = new FileInfo(path);
            if (fi.Length < 64 || fi.Length > 512L * 1024 * 1024) return false;
            using var fs = File.OpenRead(path);
            Span<byte> hdr = stackalloc byte[8];
            if (fs.Read(hdr) < 8) return false;
            // 7F E L F , class=2 (64-bit), data=2 (big-endian)
            return hdr[0] == 0x7F && hdr[1] == (byte)'E' && hdr[2] == (byte)'L' && hdr[3] == (byte)'F'
                && hdr[4] == 2 && hdr[5] == 2;
        }
        catch { return false; }
    }
}
