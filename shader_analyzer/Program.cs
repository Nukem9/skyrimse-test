using System;
using System.IO;
using System.Windows.Forms;

namespace shader_analyzer
{
    static class Program
    {
        private static FormMain m_MainForm;
        public static string ShaderDumpDirectory;
        public static string ShaderSourceDirectory;
        public static string ShaderDiffDirectory;

        /// <summary>
        /// The main entry point for the application.
        /// </summary>
        [STAThread]
        static void Main()
        {
            Application.EnableVisualStyles();
            Application.SetCompatibleTextRenderingDefault(false);
            m_MainForm = new FormMain();

            ShaderDecompiler.DecompileShader(@"C:\myfolder_0\RunGrass\RunGrass_FlatL_1.ps.hlsl", "C:\\atest.txt", @"C:\myfolder_0\RunGrass\RunGrass_FlatL_1.ps.txt");

            //var test = new FXPPackageExtractor(@"E:\Projects\Skyrim\SkyrimSETest\Resources\SSE_PS4_Shaders011.fxp", FXPPackageExtractor.PackageType.SkyrimSpecialEditionPS4);
            //test.Extract();

            // Create the temporary folders to hold our garbage in the meantime
            ShaderDumpDirectory = "C:\\ShaderDump";
            ShaderSourceDirectory = "C:\\ShaderSource";
            ShaderDiffDirectory = "C:\\ShaderDiff";

            Directory.CreateDirectory(ShaderDumpDirectory);
            Directory.CreateDirectory(ShaderSourceDirectory);
            Directory.CreateDirectory(ShaderDiffDirectory);

            LogLine("Working directory: {0}", Environment.CurrentDirectory);
            LogLine("Shader source directory: {0}", ShaderSourceDirectory);
            LogLine("Shader dump directory: {0}", ShaderDumpDirectory);
            LogLine("Diff file directory: {0}", ShaderDiffDirectory);
            LogLine("");

            Application.Run(m_MainForm);
        }

        public static void Log(string Format, params object[] Parameters)
        {
            if (m_MainForm.InvokeRequired)
            {
                m_MainForm.Invoke(new Action(() => m_MainForm.Log(Format, Parameters)));
            }
            else
            {
                m_MainForm.Log(Format, Parameters);
            }
        }

        public static void LogLine(string Format, params object[] Parameters)
        {
            if (m_MainForm.InvokeRequired)
            {
                m_MainForm.Invoke(new Action(() => m_MainForm.LogLine(Format, Parameters)));
            }
            else
            {
                m_MainForm.LogLine(Format, Parameters);
            }
        }
    }
}
