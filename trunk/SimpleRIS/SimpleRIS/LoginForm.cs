using System;
using System.Windows.Forms;

using DevExpress.XtraEditors;
using DevExpress.XtraEditors.Controls;

namespace SimpleRIS
{
    public partial class LoginForm : BaseForm
    {
        public LoginForm()
        {
            InitializeComponent();

            changeSkinState(this.checkEditSkin.Checked);
            
            ComboBoxItemCollection coll = this.comboBoxEditSkin.Properties.Items;
            coll.BeginUpdate();
            try {
                foreach (DevExpress.Skins.SkinContainer skin in DevExpress.Skins.SkinManager.Default.Skins)
                    coll.Add(skin.SkinName);
            }
            finally
            {
                coll.EndUpdate();
            }
            this.comboBoxEditSkin.SelectedIndex = 0;
        }

        private void changeSkinState(bool enableSkin)
        {
            if (enableSkin)
            {
                BaseForm.defaultLookAndFeel.LookAndFeel.SetSkinStyle(this.comboBoxEditSkin.Text);
                this.comboBoxEditSkin.Enabled = true;
            }
            else
            {
                BaseForm.defaultLookAndFeel.LookAndFeel.SetWindowsXPStyle();
                this.comboBoxEditSkin.Enabled = false;
            }
        }

        private void simpleButtonOK_Click(object sender, EventArgs e)
        {
            Properties.Settings instance = Properties.Settings.Default;
            try
            {
                instance.modifyUserAndPassword(this.textEditUserId.Text, this.textEditPassword.Text);
                Program.UserId = this.textEditUserId.Text;
                Program.Password = this.textEditPassword.Text;
                Program.RunResult = ApplicationResult.CONTINUE;
                this.Close();
            }
            catch (Exception dbConnectingException)
            {
                MessageBox.Show(dbConnectingException.Message, "登录失败");
            }
        }

        private void simpleButtonCancel_Click(object sender, EventArgs e)
        {
            Program.RunResult = ApplicationResult.EXIT;
            this.Close();
        }

        private void comboBoxEditSkin_SelectedIndexChanged(object sender, EventArgs e)
        {
            BaseForm.defaultLookAndFeel.LookAndFeel.SkinName = this.comboBoxEditSkin.Text;
        }

        private void checkEditSkin_CheckedChanged(object sender, EventArgs e)
        {
            changeSkinState(((CheckEdit)sender).Checked);
        }

        private void LoginForm_FormClosing(object sender, FormClosingEventArgs e)
        {
            Properties.Settings.Default.Save();
        }
    }
}