namespace PS3Recomp.Gui;

/// <summary>
/// Default DualShock 3 → keyboard mapping (host keys).
/// Persisted under settings.json KeyMap section; editable later in a Key Mapping UI phase.
/// </summary>
public sealed class KeyMap
{
    // Analog sticks
    public string LeftStickLeft { get; set; } = "Left";
    public string LeftStickDown { get; set; } = "Down";
    public string LeftStickRight { get; set; } = "Right";
    public string LeftStickUp { get; set; } = "Up";
    public string RightStickLeft { get; set; } = "H";
    public string RightStickDown { get; set; } = "J";
    public string RightStickRight { get; set; } = "K";
    public string RightStickUp { get; set; } = "U";

    // System
    public string Start { get; set; } = "V";
    public string Select { get; set; } = "Space";
    public string PsButton { get; set; } = "Backspace";

    // Face buttons (PS layout)
    public string Square { get; set; } = "A";
    public string Cross { get; set; } = "Z";
    public string Circle { get; set; } = "X";
    public string Triangle { get; set; } = "S";

    // D-Pad (numpad)
    public string DpadLeft { get; set; } = "NumPad4";
    public string DpadDown { get; set; } = "NumPad5";
    public string DpadRight { get; set; } = "NumPad6";
    public string DpadUp { get; set; } = "NumPad8";

    // Shoulders / triggers / stick clicks
    public string R1 { get; set; } = "R";
    public string R2 { get; set; } = "RControlKey";
    public string R3 { get; set; } = "LControlKey";
    public string L1 { get; set; } = "L";
    public string L2 { get; set; } = "RShiftKey";
    public string L3 { get; set; } = "LShiftKey";

    public static KeyMap Defaults() => new();
}
