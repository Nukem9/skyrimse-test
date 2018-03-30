namespace shader_analyzer
{
    partial class FormMain
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
            this.richTextBoxLog = new System.Windows.Forms.RichTextBox();
            this.checkBoxRecompile3DMigoto = new System.Windows.Forms.CheckBox();
            this.checkBoxIgnoreCodeMismatch = new System.Windows.Forms.CheckBox();
            this.groupBox1 = new System.Windows.Forms.GroupBox();
            this.checkBoxWater = new System.Windows.Forms.CheckBox();
            this.checkBoxUtility = new System.Windows.Forms.CheckBox();
            this.checkBoxSky = new System.Windows.Forms.CheckBox();
            this.checkBoxRunGrass = new System.Windows.Forms.CheckBox();
            this.checkBoxParticle = new System.Windows.Forms.CheckBox();
            this.checkBoxLighting = new System.Windows.Forms.CheckBox();
            this.checkBoxEffect = new System.Windows.Forms.CheckBox();
            this.checkBoxDistantTree = new System.Windows.Forms.CheckBox();
            this.checkBoxBloodSplatter = new System.Windows.Forms.CheckBox();
            this.checkBoxCompareDisasm = new System.Windows.Forms.CheckBox();
            this.buttonRun = new System.Windows.Forms.Button();
            this.groupBox2 = new System.Windows.Forms.GroupBox();
            this.listViewShaders = new System.Windows.Forms.ListView();
            this.groupBox3 = new System.Windows.Forms.GroupBox();
            this.groupBox1.SuspendLayout();
            this.groupBox2.SuspendLayout();
            this.groupBox3.SuspendLayout();
            this.SuspendLayout();
            // 
            // richTextBoxLog
            // 
            this.richTextBoxLog.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
            | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.richTextBoxLog.Location = new System.Drawing.Point(12, 383);
            this.richTextBoxLog.Name = "richTextBoxLog";
            this.richTextBoxLog.ReadOnly = true;
            this.richTextBoxLog.Size = new System.Drawing.Size(999, 204);
            this.richTextBoxLog.TabIndex = 0;
            this.richTextBoxLog.Text = "";
            // 
            // checkBoxRecompile3DMigoto
            // 
            this.checkBoxRecompile3DMigoto.AutoSize = true;
            this.checkBoxRecompile3DMigoto.Checked = true;
            this.checkBoxRecompile3DMigoto.CheckState = System.Windows.Forms.CheckState.Checked;
            this.checkBoxRecompile3DMigoto.Location = new System.Drawing.Point(6, 65);
            this.checkBoxRecompile3DMigoto.Name = "checkBoxRecompile3DMigoto";
            this.checkBoxRecompile3DMigoto.Size = new System.Drawing.Size(277, 17);
            this.checkBoxRecompile3DMigoto.TabIndex = 1;
            this.checkBoxRecompile3DMigoto.Text = "Recompile and validate 3DMigoto\'s decompiled code";
            this.checkBoxRecompile3DMigoto.UseVisualStyleBackColor = true;
            // 
            // checkBoxIgnoreCodeMismatch
            // 
            this.checkBoxIgnoreCodeMismatch.AutoSize = true;
            this.checkBoxIgnoreCodeMismatch.Location = new System.Drawing.Point(6, 19);
            this.checkBoxIgnoreCodeMismatch.Name = "checkBoxIgnoreCodeMismatch";
            this.checkBoxIgnoreCodeMismatch.Size = new System.Drawing.Size(260, 17);
            this.checkBoxIgnoreCodeMismatch.TabIndex = 2;
            this.checkBoxIgnoreCodeMismatch.Text = "Only compare shader header and type information";
            this.checkBoxIgnoreCodeMismatch.UseVisualStyleBackColor = true;
            // 
            // groupBox1
            // 
            this.groupBox1.Controls.Add(this.checkBoxWater);
            this.groupBox1.Controls.Add(this.checkBoxUtility);
            this.groupBox1.Controls.Add(this.checkBoxSky);
            this.groupBox1.Controls.Add(this.checkBoxRunGrass);
            this.groupBox1.Controls.Add(this.checkBoxParticle);
            this.groupBox1.Controls.Add(this.checkBoxLighting);
            this.groupBox1.Controls.Add(this.checkBoxEffect);
            this.groupBox1.Controls.Add(this.checkBoxDistantTree);
            this.groupBox1.Controls.Add(this.checkBoxBloodSplatter);
            this.groupBox1.Location = new System.Drawing.Point(12, 12);
            this.groupBox1.Name = "groupBox1";
            this.groupBox1.Size = new System.Drawing.Size(122, 236);
            this.groupBox1.TabIndex = 3;
            this.groupBox1.TabStop = false;
            this.groupBox1.Text = "Shader Types";
            // 
            // checkBoxWater
            // 
            this.checkBoxWater.AutoSize = true;
            this.checkBoxWater.Checked = true;
            this.checkBoxWater.CheckState = System.Windows.Forms.CheckState.Checked;
            this.checkBoxWater.Location = new System.Drawing.Point(7, 212);
            this.checkBoxWater.Name = "checkBoxWater";
            this.checkBoxWater.Size = new System.Drawing.Size(55, 17);
            this.checkBoxWater.TabIndex = 8;
            this.checkBoxWater.Text = "Water";
            this.checkBoxWater.UseVisualStyleBackColor = true;
            // 
            // checkBoxUtility
            // 
            this.checkBoxUtility.AutoSize = true;
            this.checkBoxUtility.Checked = true;
            this.checkBoxUtility.CheckState = System.Windows.Forms.CheckState.Checked;
            this.checkBoxUtility.Location = new System.Drawing.Point(7, 188);
            this.checkBoxUtility.Name = "checkBoxUtility";
            this.checkBoxUtility.Size = new System.Drawing.Size(51, 17);
            this.checkBoxUtility.TabIndex = 7;
            this.checkBoxUtility.Text = "Utility";
            this.checkBoxUtility.UseVisualStyleBackColor = true;
            // 
            // checkBoxSky
            // 
            this.checkBoxSky.AutoSize = true;
            this.checkBoxSky.Checked = true;
            this.checkBoxSky.CheckState = System.Windows.Forms.CheckState.Checked;
            this.checkBoxSky.Location = new System.Drawing.Point(7, 164);
            this.checkBoxSky.Name = "checkBoxSky";
            this.checkBoxSky.Size = new System.Drawing.Size(44, 17);
            this.checkBoxSky.TabIndex = 6;
            this.checkBoxSky.Text = "Sky";
            this.checkBoxSky.UseVisualStyleBackColor = true;
            // 
            // checkBoxRunGrass
            // 
            this.checkBoxRunGrass.AutoSize = true;
            this.checkBoxRunGrass.Checked = true;
            this.checkBoxRunGrass.CheckState = System.Windows.Forms.CheckState.Checked;
            this.checkBoxRunGrass.Location = new System.Drawing.Point(7, 140);
            this.checkBoxRunGrass.Name = "checkBoxRunGrass";
            this.checkBoxRunGrass.Size = new System.Drawing.Size(73, 17);
            this.checkBoxRunGrass.TabIndex = 5;
            this.checkBoxRunGrass.Text = "RunGrass";
            this.checkBoxRunGrass.UseVisualStyleBackColor = true;
            // 
            // checkBoxParticle
            // 
            this.checkBoxParticle.AutoSize = true;
            this.checkBoxParticle.Checked = true;
            this.checkBoxParticle.CheckState = System.Windows.Forms.CheckState.Checked;
            this.checkBoxParticle.Location = new System.Drawing.Point(7, 116);
            this.checkBoxParticle.Name = "checkBoxParticle";
            this.checkBoxParticle.Size = new System.Drawing.Size(61, 17);
            this.checkBoxParticle.TabIndex = 4;
            this.checkBoxParticle.Text = "Particle";
            this.checkBoxParticle.UseVisualStyleBackColor = true;
            // 
            // checkBoxLighting
            // 
            this.checkBoxLighting.AutoSize = true;
            this.checkBoxLighting.Checked = true;
            this.checkBoxLighting.CheckState = System.Windows.Forms.CheckState.Checked;
            this.checkBoxLighting.Location = new System.Drawing.Point(7, 92);
            this.checkBoxLighting.Name = "checkBoxLighting";
            this.checkBoxLighting.Size = new System.Drawing.Size(63, 17);
            this.checkBoxLighting.TabIndex = 3;
            this.checkBoxLighting.Text = "Lighting";
            this.checkBoxLighting.UseVisualStyleBackColor = true;
            // 
            // checkBoxEffect
            // 
            this.checkBoxEffect.AutoSize = true;
            this.checkBoxEffect.Checked = true;
            this.checkBoxEffect.CheckState = System.Windows.Forms.CheckState.Checked;
            this.checkBoxEffect.Location = new System.Drawing.Point(7, 68);
            this.checkBoxEffect.Name = "checkBoxEffect";
            this.checkBoxEffect.Size = new System.Drawing.Size(54, 17);
            this.checkBoxEffect.TabIndex = 2;
            this.checkBoxEffect.Text = "Effect";
            this.checkBoxEffect.UseVisualStyleBackColor = true;
            // 
            // checkBoxDistantTree
            // 
            this.checkBoxDistantTree.AutoSize = true;
            this.checkBoxDistantTree.Checked = true;
            this.checkBoxDistantTree.CheckState = System.Windows.Forms.CheckState.Checked;
            this.checkBoxDistantTree.Location = new System.Drawing.Point(7, 44);
            this.checkBoxDistantTree.Name = "checkBoxDistantTree";
            this.checkBoxDistantTree.Size = new System.Drawing.Size(81, 17);
            this.checkBoxDistantTree.TabIndex = 1;
            this.checkBoxDistantTree.Text = "DistantTree";
            this.checkBoxDistantTree.UseVisualStyleBackColor = true;
            // 
            // checkBoxBloodSplatter
            // 
            this.checkBoxBloodSplatter.AutoSize = true;
            this.checkBoxBloodSplatter.Checked = true;
            this.checkBoxBloodSplatter.CheckState = System.Windows.Forms.CheckState.Checked;
            this.checkBoxBloodSplatter.Location = new System.Drawing.Point(7, 20);
            this.checkBoxBloodSplatter.Name = "checkBoxBloodSplatter";
            this.checkBoxBloodSplatter.Size = new System.Drawing.Size(89, 17);
            this.checkBoxBloodSplatter.TabIndex = 0;
            this.checkBoxBloodSplatter.Text = "BloodSplatter";
            this.checkBoxBloodSplatter.UseVisualStyleBackColor = true;
            // 
            // checkBoxCompareDisasm
            // 
            this.checkBoxCompareDisasm.AutoSize = true;
            this.checkBoxCompareDisasm.Checked = true;
            this.checkBoxCompareDisasm.CheckState = System.Windows.Forms.CheckState.Checked;
            this.checkBoxCompareDisasm.Location = new System.Drawing.Point(6, 42);
            this.checkBoxCompareDisasm.Name = "checkBoxCompareDisasm";
            this.checkBoxCompareDisasm.Size = new System.Drawing.Size(278, 17);
            this.checkBoxCompareDisasm.TabIndex = 4;
            this.checkBoxCompareDisasm.Text = "Compare 3DMigoto disassembly against D3DCompiler";
            this.checkBoxCompareDisasm.UseVisualStyleBackColor = true;
            // 
            // buttonRun
            // 
            this.buttonRun.Location = new System.Drawing.Point(12, 354);
            this.buttonRun.Name = "buttonRun";
            this.buttonRun.Size = new System.Drawing.Size(75, 23);
            this.buttonRun.TabIndex = 5;
            this.buttonRun.Text = "Run";
            this.buttonRun.UseVisualStyleBackColor = true;
            this.buttonRun.Click += new System.EventHandler(this.buttonRun_Click);
            // 
            // groupBox2
            // 
            this.groupBox2.Controls.Add(this.checkBoxIgnoreCodeMismatch);
            this.groupBox2.Controls.Add(this.checkBoxRecompile3DMigoto);
            this.groupBox2.Controls.Add(this.checkBoxCompareDisasm);
            this.groupBox2.Location = new System.Drawing.Point(140, 12);
            this.groupBox2.Name = "groupBox2";
            this.groupBox2.Size = new System.Drawing.Size(295, 87);
            this.groupBox2.TabIndex = 6;
            this.groupBox2.TabStop = false;
            this.groupBox2.Text = "Options";
            // 
            // listViewShaders
            // 
            this.listViewShaders.Location = new System.Drawing.Point(6, 19);
            this.listViewShaders.Name = "listViewShaders";
            this.listViewShaders.Size = new System.Drawing.Size(558, 340);
            this.listViewShaders.TabIndex = 7;
            this.listViewShaders.UseCompatibleStateImageBehavior = false;
            this.listViewShaders.DoubleClick += new System.EventHandler(this.listViewShaders_DoubleClick);
            // 
            // groupBox3
            // 
            this.groupBox3.Controls.Add(this.listViewShaders);
            this.groupBox3.Location = new System.Drawing.Point(441, 12);
            this.groupBox3.Name = "groupBox3";
            this.groupBox3.Size = new System.Drawing.Size(570, 365);
            this.groupBox3.TabIndex = 8;
            this.groupBox3.TabStop = false;
            this.groupBox3.Text = "Mismatched Shader List";
            // 
            // FormMain
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(1023, 599);
            this.Controls.Add(this.groupBox3);
            this.Controls.Add(this.groupBox2);
            this.Controls.Add(this.buttonRun);
            this.Controls.Add(this.groupBox1);
            this.Controls.Add(this.richTextBoxLog);
            this.Name = "FormMain";
            this.Text = "ShaderAnalyzer";
            this.Load += new System.EventHandler(this.Form1_Load);
            this.groupBox1.ResumeLayout(false);
            this.groupBox1.PerformLayout();
            this.groupBox2.ResumeLayout(false);
            this.groupBox2.PerformLayout();
            this.groupBox3.ResumeLayout(false);
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.RichTextBox richTextBoxLog;
        private System.Windows.Forms.CheckBox checkBoxRecompile3DMigoto;
        private System.Windows.Forms.CheckBox checkBoxIgnoreCodeMismatch;
        private System.Windows.Forms.GroupBox groupBox1;
        private System.Windows.Forms.CheckBox checkBoxWater;
        private System.Windows.Forms.CheckBox checkBoxUtility;
        private System.Windows.Forms.CheckBox checkBoxSky;
        private System.Windows.Forms.CheckBox checkBoxRunGrass;
        private System.Windows.Forms.CheckBox checkBoxParticle;
        private System.Windows.Forms.CheckBox checkBoxLighting;
        private System.Windows.Forms.CheckBox checkBoxEffect;
        private System.Windows.Forms.CheckBox checkBoxDistantTree;
        private System.Windows.Forms.CheckBox checkBoxBloodSplatter;
        private System.Windows.Forms.CheckBox checkBoxCompareDisasm;
        private System.Windows.Forms.Button buttonRun;
        private System.Windows.Forms.GroupBox groupBox2;
        private System.Windows.Forms.ListView listViewShaders;
        private System.Windows.Forms.GroupBox groupBox3;
    }
}

