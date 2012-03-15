namespace SimpleRIS
{
    partial class MainForm
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
            this.components = new System.ComponentModel.Container();
            this.dockManagerMain = new DevExpress.XtraBars.Docking.DockManager(this.components);
            this.panelContainerRegistration = new DevExpress.XtraBars.Docking.DockPanel();
            this.dockPanelRegistration = new DevExpress.XtraBars.Docking.DockPanel();
            this.dockPanelRegistration_Container = new DevExpress.XtraBars.Docking.ControlContainer();
            this.registration = new RegistrationControl();
            this.dockPanelStudy = new DevExpress.XtraBars.Docking.DockPanel();
            this.dockPanelStudy_Container = new DevExpress.XtraBars.Docking.ControlContainer();
            ((System.ComponentModel.ISupportInitialize)(this.dockManagerMain)).BeginInit();
            this.panelContainerRegistration.SuspendLayout();
            this.dockPanelRegistration.SuspendLayout();
            this.dockPanelRegistration_Container.SuspendLayout();
            this.dockPanelStudy.SuspendLayout();
            this.SuspendLayout();
            // 
            // dockManagerMain
            // 
            this.dockManagerMain.Form = this;
            this.dockManagerMain.RootPanels.AddRange(new DevExpress.XtraBars.Docking.DockPanel[] {
            this.panelContainerRegistration});
            this.dockManagerMain.TopZIndexControls.AddRange(new string[] {
            "DevExpress.XtraBars.BarDockControl",
            "DevExpress.XtraBars.StandaloneBarDockControl",
            "System.Windows.Forms.StatusBar",
            "DevExpress.XtraBars.Ribbon.RibbonStatusBar",
            "DevExpress.XtraBars.Ribbon.RibbonControl"});
            // 
            // panelContainerRegistration
            // 
            this.panelContainerRegistration.ActiveChild = this.dockPanelRegistration;
            this.panelContainerRegistration.Controls.Add(this.dockPanelRegistration);
            this.panelContainerRegistration.Controls.Add(this.dockPanelStudy);
            this.panelContainerRegistration.Dock = DevExpress.XtraBars.Docking.DockingStyle.Left;
            this.panelContainerRegistration.FloatVertical = true;
            this.panelContainerRegistration.ID = new System.Guid("e3c34fb8-da6b-4683-865d-2fceb44edf02");
            this.panelContainerRegistration.Location = new System.Drawing.Point(0, 0);
            this.panelContainerRegistration.Name = "panelContainerRegistration";
            this.panelContainerRegistration.OriginalSize = new System.Drawing.Size(420, 200);
            this.panelContainerRegistration.Size = new System.Drawing.Size(420, 533);
            this.panelContainerRegistration.Tabbed = true;
            this.panelContainerRegistration.Text = "检查";
            // 
            // dockPanelRegistration
            // 
            this.dockPanelRegistration.Controls.Add(this.dockPanelRegistration_Container);
            this.dockPanelRegistration.Dock = DevExpress.XtraBars.Docking.DockingStyle.Fill;
            this.dockPanelRegistration.FloatVertical = true;
            this.dockPanelRegistration.ID = new System.Guid("fd91a2ba-b7f3-4673-b46c-0c58b43eae91");
            this.dockPanelRegistration.Location = new System.Drawing.Point(3, 25);
            this.dockPanelRegistration.Name = "dockPanelRegistration";
            this.dockPanelRegistration.OriginalSize = new System.Drawing.Size(414, 408);
            this.dockPanelRegistration.Size = new System.Drawing.Size(414, 482);
            this.dockPanelRegistration.Text = "登记";
            // 
            // dockPanelRegistration_Container
            // 
            this.dockPanelRegistration_Container.Controls.Add(this.registration);
            this.dockPanelRegistration_Container.Location = new System.Drawing.Point(0, 0);
            this.dockPanelRegistration_Container.Name = "dockPanelRegistration_Container";
            this.dockPanelRegistration_Container.Size = new System.Drawing.Size(414, 482);
            this.dockPanelRegistration_Container.TabIndex = 0;
            // 
            // registration
            // 
            this.registration.Dock = System.Windows.Forms.DockStyle.Fill;
            this.registration.Location = new System.Drawing.Point(0, 0);
            this.registration.Name = "registration";
            this.registration.Size = new System.Drawing.Size(414, 482);
            this.registration.TabIndex = 0;
            // 
            // dockPanelStudy
            // 
            this.dockPanelStudy.Controls.Add(this.dockPanelStudy_Container);
            this.dockPanelStudy.Dock = DevExpress.XtraBars.Docking.DockingStyle.Fill;
            this.dockPanelStudy.ID = new System.Guid("46cdd950-7034-4455-a9d7-fbd08849b7e0");
            this.dockPanelStudy.Location = new System.Drawing.Point(3, 25);
            this.dockPanelStudy.Name = "dockPanelStudy";
            this.dockPanelStudy.OriginalSize = new System.Drawing.Size(414, 408);
            this.dockPanelStudy.Size = new System.Drawing.Size(414, 482);
            this.dockPanelStudy.Text = "检查";
            // 
            // dockPanelStudy_Container
            // 
            this.dockPanelStudy_Container.Location = new System.Drawing.Point(0, 0);
            this.dockPanelStudy_Container.Name = "dockPanelStudy_Container";
            this.dockPanelStudy_Container.Size = new System.Drawing.Size(414, 482);
            this.dockPanelStudy_Container.TabIndex = 0;
            // 
            // MainForm
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(7F, 14F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(792, 533);
            this.Controls.Add(this.panelContainerRegistration);
            this.Name = "MainForm";
            this.Text = "SmartRIS";
            this.WindowState = System.Windows.Forms.FormWindowState.Maximized;
            this.FormClosing += new System.Windows.Forms.FormClosingEventHandler(this.MainForm_FormClosing);
            ((System.ComponentModel.ISupportInitialize)(this.dockManagerMain)).EndInit();
            this.panelContainerRegistration.ResumeLayout(false);
            this.dockPanelRegistration.ResumeLayout(false);
            this.dockPanelRegistration_Container.ResumeLayout(false);
            this.dockPanelStudy.ResumeLayout(false);
            this.ResumeLayout(false);

        }

        #endregion

        private DevExpress.XtraBars.Docking.DockManager dockManagerMain;
        private DevExpress.XtraBars.Docking.DockPanel dockPanelRegistration;
        private DevExpress.XtraBars.Docking.ControlContainer dockPanelRegistration_Container;
        private DevExpress.XtraBars.Docking.DockPanel panelContainerRegistration;
        private DevExpress.XtraBars.Docking.DockPanel dockPanelStudy;
        private DevExpress.XtraBars.Docking.ControlContainer dockPanelStudy_Container;
        private RegistrationControl registration;
    }
}