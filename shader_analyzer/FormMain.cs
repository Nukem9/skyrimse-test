using System;
using System.Windows.Forms;

namespace shader_analyzer
{
    public partial class FormMain : Form
    {
        public FormMain()
        {
            InitializeComponent();
        }

        private void Form1_Load(object sender, EventArgs e)
        {
        }

        public void Log(string Format, params object[] Parameters)
        {
            richTextBoxLog.Text += string.Format(Format, Parameters);

            richTextBoxLog.SelectionStart = richTextBoxLog.Text.Length;
            richTextBoxLog.ScrollToCaret();
        }

        public void LogLine(string Format, params object[] Parameters)
        {
            Log(Format + Environment.NewLine, Parameters);
        }

        private void buttonRun_Click(object sender, EventArgs e)
        {
            ShaderAnalyzer.DoStuff();
        }
    }
}
