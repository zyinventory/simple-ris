using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.IO;
using System.Text;
using System.Windows.Forms;
using DevExpress.XtraEditors;

namespace SimpleRIS
{
    public partial class MainForm : BaseForm
    {
        private const string dockLayoutFile = "dockLayout.xml";

        public MainForm()
        {
            InitializeComponent();
            FileInfo fi = new FileInfo(dockLayoutFile);
            if (fi.Exists)
            {
                this.dockManagerMain.BeginUpdate();
                this.dockManagerMain.RestoreLayoutFromXml(dockLayoutFile);
                this.dockManagerMain.EndUpdate();
            }
        }

        private void MainForm_FormClosing(object sender, FormClosingEventArgs e)
        {
            this.dockManagerMain.SaveLayoutToXml(dockLayoutFile);
        }
    }
}