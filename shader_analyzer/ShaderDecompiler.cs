using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;

namespace shader_analyzer
{
    class ShaderDecompiler
    {
        public static void DecompileShader(string InputFile, string OutputFile, string MetadataFile = null)
        {
            // Create a copy of the file since it will be trashed
            if (!InputFile.Equals(OutputFile))
                File.Copy(InputFile, OutputFile, true);

            var p = new System.Diagnostics.Process();
            p.StartInfo.WorkingDirectory = "C:\\Users\\Administrator\\Desktop\\cmd_Decompiler-1.3.2\\";
            p.StartInfo.FileName = "C:\\Users\\Administrator\\Desktop\\cmd_Decompiler-1.3.2\\cmd_Decompiler.exe";
            p.StartInfo.Arguments = "-D " + OutputFile;
            p.StartInfo.RedirectStandardOutput = false;
            p.StartInfo.UseShellExecute = false;
            p.StartInfo.CreateNoWindow = true;
            p.Start();

            // Gather metadata while it's being decompiled
            ShaderMetadata metadata = null;
            ShaderVariableMetadata variables = null;

            if (!string.IsNullOrEmpty(MetadataFile))
            {
                metadata = new ShaderMetadata(MetadataFile);
                variables = new ShaderVariableMetadata(MetadataFile);
            }

            p.WaitForExit();

            // Fix stupid naming quirk with 3dm (swap the files)
            string ext = Path.GetExtension(OutputFile);

            if (!ext.ToLower().Equals(".hlsl"))
            {
                string target = OutputFile.Replace(Path.GetExtension(OutputFile), ".hlsl");
                File.Delete(OutputFile);
                File.Move(target, OutputFile);
            }

            if (metadata != null)
            {
                // Rip out 3dmigoto's header - it ends right before the "void main()" line
                List<string> fileLines = System.IO.File.ReadAllLines(OutputFile).ToList();

                for (int i = 0; i < fileLines.Count; i++)
                {
                    if (fileLines[i].ToLower().Contains("void main"))
                        break;

                    fileLines.Remove(fileLines[i]);
                    i--;
                }

                // Now paste our header back into it
                List<string> replacedLines = new List<string>(metadata.GetTextData());

                // Regenerate samplers and texture slots
                foreach (Tuple<int, string> sampler in metadata.GetSamplers())
                {
                    ReplaceInAllLines(fileLines, $"s{sampler.Item1}_s", $"{sampler.Item2}");
                    ReplaceInAllLines(fileLines, $"t{sampler.Item1}.", $"Tex{sampler.Item2}.");

                    replacedLines.Add($"SamplerState {sampler.Item2} : register(s{sampler.Item1});");
                    replacedLines.Add($"Texture2D<float4> Tex{sampler.Item2} : register(t{sampler.Item1});");
                }

                // Regenerate constant buffer variable accesses (TODO: inefficient as hell)
                for (int i = 0; i < fileLines.Count; i++)
                {

                }

                // Add back HLSL instructions
                replacedLines.Add(Environment.NewLine);
                replacedLines.AddRange(fileLines);

                // Done. Dump it back to disk.
                File.WriteAllLines(OutputFile, replacedLines);
            }
        }

        private static void ReplaceInAllLines(List<string> Lines, string Search, string Replace)
        {
            for (int i = 0; i < Lines.Count; i++)
                Lines[i] = Lines[i].Replace(Search, Replace);
        }
    }
}
