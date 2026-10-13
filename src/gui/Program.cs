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

        Application.Run(new MainForm());
    }
}
