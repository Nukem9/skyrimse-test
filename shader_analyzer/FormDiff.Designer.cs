namespace shader_analyzer
{
    partial class FormDiff
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
            this.richTextBoxSide1 = new System.Windows.Forms.RichTextBox();
            this.richTextBoxSide2 = new System.Windows.Forms.RichTextBox();
            this.SuspendLayout();
            // 
            // richTextBoxSide1
            // 
            this.richTextBoxSide1.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
            | System.Windows.Forms.AnchorStyles.Left)));
            this.richTextBoxSide1.Location = new System.Drawing.Point(12, 12);
            this.richTextBoxSide1.Name = "richTextBoxSide1";
            this.richTextBoxSide1.Size = new System.Drawing.Size(682, 895);
            this.richTextBoxSide1.TabIndex = 0;
            this.richTextBoxSide1.Text = "";
            // 
            // richTextBoxSide2
            // 
            this.richTextBoxSide2.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
            | System.Windows.Forms.AnchorStyles.Left)));
            this.richTextBoxSide2.Location = new System.Drawing.Point(698, 12);
            this.richTextBoxSide2.Name = "richTextBoxSide2";
            this.richTextBoxSide2.Size = new System.Drawing.Size(682, 895);
            this.richTextBoxSide2.TabIndex = 1;
            this.richTextBoxSide2.Text = "";
            // 
            // FormDiff
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(1392, 919);
            this.Controls.Add(this.richTextBoxSide2);
            this.Controls.Add(this.richTextBoxSide1);
            this.Name = "FormDiff";
            this.Text = "Diff View";
            this.FormClosing += new System.Windows.Forms.FormClosingEventHandler(this.FormDiff_Close);
            this.Load += new System.EventHandler(this.FormDiff_Load);
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.RichTextBox richTextBoxSide1;
        private System.Windows.Forms.RichTextBox richTextBoxSide2;
    }
}