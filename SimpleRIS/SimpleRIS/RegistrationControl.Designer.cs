namespace SimpleRIS
{
    partial class RegistrationControl
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

        #region Component Designer generated code

        /// <summary> 
        /// Required method for Designer support - do not modify 
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            this.splitLevel1 = new DevExpress.XtraEditors.SplitContainerControl();
            this.splitLevel2 = new DevExpress.XtraEditors.SplitContainerControl();
            this.searchCondition = new SimpleRIS.RegistrationSearchCondition();
            ((System.ComponentModel.ISupportInitialize)(this.splitLevel1)).BeginInit();
            this.splitLevel1.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.splitLevel2)).BeginInit();
            this.splitLevel2.SuspendLayout();
            this.SuspendLayout();
            // 
            // splitLevel1
            // 
            this.splitLevel1.Dock = System.Windows.Forms.DockStyle.Fill;
            this.splitLevel1.Horizontal = false;
            this.splitLevel1.Location = new System.Drawing.Point(0, 0);
            this.splitLevel1.Name = "splitLevel1";
            this.splitLevel1.Panel1.Controls.Add(this.splitLevel2);
            this.splitLevel1.Panel1.Text = "Panel1";
            this.splitLevel1.Panel2.Text = "Panel2";
            this.splitLevel1.Size = new System.Drawing.Size(409, 469);
            this.splitLevel1.SplitterPosition = 312;
            this.splitLevel1.TabIndex = 0;
            this.splitLevel1.Text = "splitContainerControl1";
            // 
            // splitLevel2
            // 
            this.splitLevel2.Dock = System.Windows.Forms.DockStyle.Fill;
            this.splitLevel2.Horizontal = false;
            this.splitLevel2.Location = new System.Drawing.Point(0, 0);
            this.splitLevel2.Name = "splitLevel2";
            this.splitLevel2.Panel1.Controls.Add(this.searchCondition);
            this.splitLevel2.Panel1.Text = "Panel1";
            this.splitLevel2.Panel2.Text = "Panel2";
            this.splitLevel2.Size = new System.Drawing.Size(409, 312);
            this.splitLevel2.SplitterPosition = 142;
            this.splitLevel2.TabIndex = 0;
            this.splitLevel2.Text = "splitContainerControl1";
            // 
            // searchCondition
            // 
            this.searchCondition.Dock = System.Windows.Forms.DockStyle.Fill;
            this.searchCondition.Location = new System.Drawing.Point(0, 0);
            this.searchCondition.Name = "searchCondition";
            this.searchCondition.Size = new System.Drawing.Size(409, 142);
            this.searchCondition.TabIndex = 0;
            // 
            // RegistrationControl
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(7F, 14F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this.splitLevel1);
            this.Name = "RegistrationControl";
            this.Size = new System.Drawing.Size(409, 469);
            ((System.ComponentModel.ISupportInitialize)(this.splitLevel1)).EndInit();
            this.splitLevel1.ResumeLayout(false);
            ((System.ComponentModel.ISupportInitialize)(this.splitLevel2)).EndInit();
            this.splitLevel2.ResumeLayout(false);
            this.ResumeLayout(false);

        }

        #endregion

        private DevExpress.XtraEditors.SplitContainerControl splitLevel1;
        private DevExpress.XtraEditors.SplitContainerControl splitLevel2;
        private RegistrationSearchCondition searchCondition;
    }
}
