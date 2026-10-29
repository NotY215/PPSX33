namespace PS3Recomp.Gui;

internal static class Program
{
    [STAThread]
    private static void Main()
    {
        ApplicationConfiguration.Initialize();

        var settings = Settings.Load();
        if (!settings.TermsAccepted)
        {
            using var terms = new TermsForm();
            var result = terms.ShowDialog();
            if (result != DialogResult.OK || !terms.Accepted)
            {
                // Leave the app — do not open main UI
                return;
            }
            settings.TermsAccepted = true;
            settings.TermsAcceptedBy = TermsForm.WindowsUserName;
            settings.TermsVersion = "2026-10-13";
            settings.Save();
        }

        // Permanent research notice: SPU is incomplete (mailbox HLE only).
        // Shown every launch so users know to report missing SPU coverage.
        MessageBox.Show(
            "SPU research status\n\n" +
            "PPSX33 currently implements only a minimal SPU path:\n" +
            "  • Mailbox HLE / auto-complete\n" +
            "  • No full SPU ISA interpreter or JIT\n" +
            "  • No vector / floating-point SPU ops\n" +
            "  • No MFC DMA, SPURS, or realistic scheduling\n\n" +
            "If you hit a game that needs a specific SPU behaviour\n" +
            "(or see SPU-related stalls in the log), please open a\n" +
            "GitHub issue with the category \"SPU coverage\" and attach\n" +
            "your latest run log. We prioritise reported SPU gaps.\n\n" +
            "This is expected research software — not a complete emulator.",
            "PPSX33 — SPU notice",
            MessageBoxButtons.OK,
            MessageBoxIcon.Information);

        Application.Run(new MainForm());
    }
}
