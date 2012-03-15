using System;
using System.Collections.Generic;
using System.Linq;
using System.Windows.Forms;

namespace SimpleRIS
{
    enum ApplicationResult { CONTINUE, EXIT, RESTART };

    static class Program
    {
        static public string UserId, Password;
        static public ApplicationResult RunResult = ApplicationResult.EXIT;

        /// <summary>
        /// 应用程序的主入口点。
        /// </summary>
        [STAThread]
        static void Main()
        {
            DevExpress.UserSkins.BonusSkins.Register();
            DevExpress.UserSkins.OfficeSkins.Register();

            Application.EnableVisualStyles();
            Application.SetCompatibleTextRenderingDefault(false);

            Application.Run(new LoginForm());
            switch (RunResult)
            {
                case ApplicationResult.CONTINUE:
                    Application.Run(new MainForm());
                    break;
                case ApplicationResult.RESTART:
                case ApplicationResult.EXIT:
                default:
                    return;
            }

            // Run MainForm result
            switch (RunResult)
            {
                case ApplicationResult.RESTART:
                    Application.Restart();
                    break;
                case ApplicationResult.CONTINUE:
                case ApplicationResult.EXIT:
                default:
                    break;
            }
        }
    }
}
