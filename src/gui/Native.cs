using System.Runtime.InteropServices;
using System.Text;

namespace PS3Recomp.Gui;

/// <summary>P/Invoke wrapper for ps3core.dll (see src/core/ps3core.h). Keep both in sync.</summary>
internal static class Native
{
    private const string Dll = "ps3core";

    [DllImport(Dll, CallingConvention = CallingConvention.Cdecl)]
    private static extern int ps3_core_version();

    [DllImport(Dll, CallingConvention = CallingConvention.Cdecl)]
    private static extern int ps3_create_project(
        [MarshalAs(UnmanagedType.LPUTF8Str)] string root,
        [MarshalAs(UnmanagedType.LPUTF8Str)] string game,
        [MarshalAs(UnmanagedType.LPUTF8Str)] string elf,
        byte[] projectDir, int projectDirLen, byte[] err, int errLen);

    [DllImport(Dll, CallingConvention = CallingConvention.Cdecl)]
    private static extern int ps3_lift_project(
        [MarshalAs(UnmanagedType.LPUTF8Str)] string projectDir, byte[] log, int logLen);

    [DllImport(Dll, CallingConvention = CallingConvention.Cdecl)]
    private static extern int ps3_build_project(
        [MarshalAs(UnmanagedType.LPUTF8Str)] string projectDir,
        [MarshalAs(UnmanagedType.LPUTF8Str)] string runtimeDir,
        [MarshalAs(UnmanagedType.LPUTF8Str)] string compilersDir,
        byte[] log, int logLen);

    private static string Str(byte[] b)
    {
        int n = Array.IndexOf(b, (byte)0);
        return Encoding.UTF8.GetString(b, 0, n < 0 ? b.Length : n);
    }

    public static int Version() => ps3_core_version();

    public static bool CreateProject(string root, string game, string elf, out string projectDir, out string error)
    {
        var d = new byte[4096]; var e = new byte[4096];
        int rc = ps3_create_project(root, game, elf, d, d.Length, e, e.Length);
        projectDir = Str(d); error = Str(e);
        return rc == 0;
    }

    public static bool Lift(string projectDir, out string log)
    {
        var l = new byte[1 << 16];
        int rc = ps3_lift_project(projectDir, l, l.Length);
        log = Str(l); return rc == 0;
    }

    public static bool Build(string projectDir, string runtimeDir, string compilersDir, out string log)
    {
        var l = new byte[1 << 20];
        int rc = ps3_build_project(projectDir, runtimeDir, compilersDir, l, l.Length);
        log = Str(l); return rc == 0;
    }
}
