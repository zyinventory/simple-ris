namespace SimpleRIS
{
    partial class LoginForm
    {
        /// <summary>
        /// Required designer variable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary>
        /// Clean up any resources being used.
        /// </summary>
        /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Windows Form Designer generated code

        /// <summary>
        /// Required method for Designer support - do not modify
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            this.textEditUserId = new DevExpress.XtraEditors.TextEdit();
            this.textEditPassword = new DevExpress.XtraEditors.TextEdit();
            this.labelControlUserId = new DevExpress.XtraEditors.LabelControl();
            this.labelControlPassword = new DevExpress.XtraEditors.LabelControl();
            this.simpleButtonOK = new DevExpress.XtraEditors.SimpleButton();
            this.simpleButtonCancel = new DevExpress.XtraEditors.SimpleButton();
            this.comboBoxEditSkin = new DevExpress.XtraEditors.ComboBoxEdit();
            this.checkEditSkin = new DevExpress.XtraEditors.CheckEdit();
            ((System.ComponentModel.ISupportInitialize)(this.textEditUserId.Properties)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.textEditPassword.Properties)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.comboBoxEditSkin.Properties)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.checkEditSkin.Properties)).BeginInit();
            this.SuspendLayout();
            // 
            // textEditUserId
            // 
            this.textEditUserId.EnterMoveNextControl = true;
            this.textEditUserId.Location = new System.Drawing.Point(55, 25);
            this.textEditUserId.Name = "textEditUserId";
            this.textEditUserId.Size = new System.Drawing.Size(152, 21);
            this.textEditUserId.TabIndex = 0;
            // 
            // textEditPassword
            // 
            this.textEditPassword.Location = new System.Drawing.Point(55, 52);
            this.textEditPassword.Name = "textEditPassword";
            this.textEditPassword.Properties.PasswordChar = '*';
            this.textEditPassword.Size = new System.Drawing.Size(152, 21);
            this.textEditPassword.TabIndex = 1;
            // 
            // labelControlUserId
            // 
            this.labelControlUserId.Location = new System.Drawing.Point(13, 28);
            this.labelControlUserId.Name = "labelControlUserId";
            this.labelControlUserId.Size = new System.Drawing.Size(36, 14);
            this.labelControlUserId.TabIndex = 2;
            this.labelControlUserId.Text = "用户：";
            // 
            // labelControlPassword
            // 
            this.labelControlPassword.Location = new System.Drawing.Point(13, 55);
            this.labelControlPassword.Name = "labelControlPassword";
            this.labelControlPassword.Size = new System.Drawing.Size(36, 14);
            this.labelControlPassword.TabIndex = 3;
            this.labelControlPassword.Text = "密码：";
            // 
            // simpleButtonOK
            // 
            this.simpleButtonOK.Location = new System.Drawing.Point(13, 79);
            this.simpleButtonOK.Name = "simpleButtonOK";
            this.simpleButtonOK.Size = new System.Drawing.Size(75, 23);
            this.simpleButtonOK.TabIndex = 3;
            this.simpleButtonOK.Text = "登录";
            this.simpleButtonOK.Click += new System.EventHandler(this.simpleButtonOK_Click);
            // 
            // simpleButtonCancel
            // 
            this.simpleButtonCancel.DialogResult = System.Windows.Forms.DialogResult.Cancel;
            this.simpleButtonCancel.Location = new System.Drawing.Point(132, 79);
            this.simpleButtonCancel.Name = "simpleButtonCancel";
            this.simpleButtonCancel.Size = new System.Drawing.Size(75, 23);
            this.simpleButtonCancel.TabIndex = 4;
            this.simpleButtonCancel.Text = "取消";
            this.simpleButtonCancel.Click += new System.EventHandler(this.simpleButtonCancel_Click);
            // 
            // comboBoxEditSkin
            // 
            this.comboBoxEditSkin.Location = new System.Drawing.Point(69, 108);
            this.comboBoxEditSkin.Name = "comboBoxEditSkin";
            this.comboBoxEditSkin.Properties.Buttons.AddRange(new DevExpress.XtraEditors.Controls.EditorButton[] {
            new DevExpress.XtraEditors.Controls.EditorButton(DevExpress.XtraEditors.Controls.ButtonPredefines.Combo)});
            this.comboBoxEditSkin.Properties.TextEditStyle = DevExpress.XtraEditors.Controls.TextEditStyles.DisableTextEditor;
            this.comboBoxEditSkin.Size = new System.Drawing.Size(138, 21);
            this.comboBoxEditSkin.TabIndex = 6;
            this.comboBoxEditSkin.SelectedIndexChanged += new System.EventHandler(this.comboBoxEditSkin_SelectedIndexChanged);
            // 
            // checkEditSkin
            // 
            this.checkEditSkin.DataBindings.Add(new System.Windows.Forms.Binding("EditValue", global::SimpleRIS.Properties.Settings.Default, "EnableSkin", true, System.Windows.Forms.DataSourceUpdateMode.OnPropertyChanged));
            this.checkEditSkin.EditValue = global::SimpleRIS.Properties.Settings.Default.EnableSkin;
            this.checkEditSkin.Location = new System.Drawing.Point(13, 111);
            this.checkEditSkin.Name = "checkEditSkin";
            this.checkEditSkin.Properties.Caption = "样式：";
            this.checkEditSkin.Size = new System.Drawing.Size(50, 19);
            this.checkEditSkin.TabIndex = 5;
            this.checkEditSkin.CheckedChanged += new System.EventHandler(this.checkEditSkin_CheckedChanged);
            // 
            // LoginForm
            // 
            this.AcceptButton = this.simpleButtonOK;
            this.AutoScaleDimensions = new System.Drawing.SizeF(7F, 14F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.CancelButton = this.simpleButtonCancel;
            this.ClientSize = new System.Drawing.Size(219, 145);
            this.Controls.Add(this.checkEditSkin);
            this.Controls.Add(this.comboBoxEditSkin);
            this.Controls.Add(this.simpleButtonCancel);
            this.Controls.Add(this.simpleButtonOK);
            this.Controls.Add(this.labelControlPassword);
            this.Controls.Add(this.labelControlUserId);
            this.Controls.Add(this.textEditPassword);
            this.Controls.Add(this.textEditUserId);
            this.Name = "LoginForm";
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterScreen;
            this.Text = "登录";
            this.FormClosing += new System.Windows.Forms.FormClosingEventHandler(this.LoginForm_FormClosing);
            ((System.ComponentModel.ISupportInitialize)(this.textEditUserId.Properties)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.textEditPassword.Properties)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.comboBoxEditSkin.Properties)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.checkEditSkin.Properties)).EndInit();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private DevExpress.XtraEditors.TextEdit textEditUserId;
        private DevExpress.XtraEditors.TextEdit textEditPassword;
        private DevExpress.XtraEditors.LabelControl labelControlUserId;
        private DevExpress.XtraEditors.LabelControl labelControlPassword;
        private DevExpress.XtraEditors.SimpleButton simpleButtonOK;
        private DevExpress.XtraEditors.SimpleButton simpleButtonCancel;
        private DevExpress.XtraEditors.ComboBoxEdit comboBoxEditSkin;
        private DevExpress.XtraEditors.CheckEdit checkEditSkin;
    }
}