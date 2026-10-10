using System.Diagnostics;

namespace PS3Recomp.Gui;

/// <summary>
/// Uses RPCS3 only to decrypt PS3 binaries (Utilities equivalent via --decrypt CLI).
/// Does not boot or play games inside RPCS3.
/// </summary>
internal static class Rpcs3Launcher
{
    /// <summary>
    /// Run: rpcs3.exe --decrypt "EBOOT.BIN"
    /// Waits for EBOOT.elf (or same basename .elf) next to the BIN, then returns its path.
    /// </summary>
    public static bool DecryptBinary(string rpcs3Exe, string ebootBinPath, out string elfPath, out string error, int timeoutMs = 120_000)
    {
        elfPath = "";
        error = "";
        try
        {
            if (!File.Exists(rpcs3Exe))
            {
                error = "rpcs3.exe not found. Set the path when prompted.";
                return false;
            }
            if (!File.Exists(ebootBinPath))
            {
                error = "EBOOT/binary not found: " + ebootBinPath;
                return false;
            }

            string? dir = Path.GetDirectoryName(ebootBinPath);
            if (string.IsNullOrEmpty(dir))
            {
                error = "Cannot resolve directory of binary.";
                return false;
            }

            string baseName = Path.GetFileNameWithoutExtension(ebootBinPath);
            // RPCS3 writes decrypted output with .elf extension beside the input
            string[] candidates =
            {
                Path.Combine(dir, baseName + ".elf"),
                Path.Combine(dir, "EBOOT.elf"),
                Path.Combine(dir, baseName + ".ELF"),
                Path.Combine(dir, "EBOOT.ELF"),
            };

            // Snapshot existing mtimes so we prefer a freshly written file
            var before = new Dictionary<string, DateTime>(StringComparer.OrdinalIgnoreCase);
            foreach (var c in candidates)
            {
                if (File.Exists(c))
                    before[c] = File.GetLastWriteTimeUtc(c);
            }

            var psi = new ProcessStartInfo
            {
                FileName = rpcs3Exe,
                Arguments = $"--decrypt \"{ebootBinPath}\"",
                UseShellExecute = false,
                CreateNoWindow = false,
                RedirectStandardOutput = true,
                RedirectStandardError = true,
                WorkingDirectory = Path.GetDirectoryName(rpcs3Exe) ?? dir,
            };

            using var proc = Process.Start(psi);
            if (proc == null)
            {
                error = "Failed to start rpcs3.exe";
                return false;
            }

            // Drain output so the process cannot block on full pipes
            var stdout = proc.StandardOutput.ReadToEndAsync();
            var stderr = proc.StandardError.ReadToEndAsync();

            if (!proc.WaitForExit(timeoutMs))
            {
                try { proc.Kill(entireProcessTree: true); } catch { }
                error = "RPCS3 decrypt timed out. If a KLIC is required, decrypt once via Utilities -> Decrypt PS3 Binaries in the GUI.";
                return false;
            }

            string outText = "";
            try { outText = (stdout.Result ?? "") + (stderr.Result ?? ""); } catch { }

            // Prefer a newly written candidate
            string? best = null;
            DateTime bestTime = DateTime.MinValue;
            foreach (var c in candidates)
            {
                if (!File.Exists(c) || !LooksLikeElf(c)) continue;
                var t = File.GetLastWriteTimeUtc(c);
                bool isNew = !before.TryGetValue(c, out var old) || t > old;
                if (isNew && t >= bestTime) { best = c; bestTime = t; }
            }
            // Fallback: any valid candidate next to the bin
            if (best == null)
            {
                foreach (var c in candidates)
                {
                    if (File.Exists(c) && LooksLikeElf(c))
                    {
                        best = c;
                        break;
                    }
                }
            }

            if (best == null)
            {
                error = "Decrypt finished but no ELF was found next to the binary.\n"
                    + "RPCS3 output:\n" + Truncate(outText, 800)
                    + "\nIf a KLIC prompt is needed, open RPCS3 -> Utilities -> Decrypt PS3 Binaries once, then retry.";
                return false;
            }

            elfPath = best;
            return true;
        }
        catch (Exception ex)
        {
            error = ex.Message;
            return false;
        }
    }

    [Obsolete("Use DecryptBinary instead of launching the game.")]
    public static bool Launch(string rpcs3Exe, string ebootPath, out string error)
    {
        // Kept for compatibility: still only starts RPCS3 with the path (not preferred).
        error = "";
        try
        {
            if (!File.Exists(rpcs3Exe)) { error = "rpcs3.exe not found."; return false; }
            Process.Start(new ProcessStartInfo(rpcs3Exe, $"\"{ebootPath}\"") { UseShellExecute = false });
            return true;
        }
        catch (Exception ex) { error = ex.Message; return false; }
    }

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
            }
        }
        string appData = Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData);
        roots.Add(Path.Combine(appData, "rpcs3", "cache"));
        roots.Add(Path.Combine(appData, "rpcs3", "dev_hdd0", "game"));

        var hits = new List<(string path, DateTime mtime)>();
        foreach (var root in roots.Distinct(StringComparer.OrdinalIgnoreCase))
        {
            if (!Directory.Exists(root)) continue;
            try { ScanDir(root, hits, 0, 3); } catch { }
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
                bool nameHit =
                    name.Equals("EBOOT.elf", StringComparison.OrdinalIgnoreCase) ||
                    name.EndsWith(".elf", StringComparison.OrdinalIgnoreCase);
                if (!nameHit) continue;
                if (!LooksLikeElf(file)) continue;
                hits.Add((file, File.GetLastWriteTimeUtc(file)));
            }
            if (depth == maxDepth) return;
            foreach (var sub in Directory.EnumerateDirectories(dir))
            {
                string leaf = Path.GetFileName(sub);
                if (leaf.Equals("shaderlog", StringComparison.OrdinalIgnoreCase)) continue;
                if (leaf.Equals("captures", StringComparison.OrdinalIgnoreCase)) continue;
                ScanDir(sub, hits, depth + 1, maxDepth);
            }
        }
        catch { }
    }

    internal static bool LooksLikeElf(string path)
    {
        try
        {
            var fi = new FileInfo(path);
            if (fi.Length < 64 || fi.Length > 512L * 1024 * 1024) return false;
            using var fs = File.OpenRead(path);
            Span<byte> hdr = stackalloc byte[8];
            if (fs.Read(hdr) < 8) return false;
            return hdr[0] == 0x7F && hdr[1] == (byte)'E' && hdr[2] == (byte)'L' && hdr[3] == (byte)'F'
                && hdr[4] == 2 && hdr[5] == 2;
        }
        catch { return false; }
    }

    private static string Truncate(string s, int n)
        => string.IsNullOrEmpty(s) ? "" : (s.Length <= n ? s : s.Substring(0, n) + "…");
}
