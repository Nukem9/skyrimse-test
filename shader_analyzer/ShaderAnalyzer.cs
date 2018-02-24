using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text.RegularExpressions;
using System.Threading;
using SharpDX.D3DCompiler;

namespace shader_analyzer
{
    static class ShaderAnalyzer
    {
        // This removes as much as physically possible from the compiled code
        private readonly static StripFlags m_StripFlags =
            StripFlags.CompilerStripDebugInformation |
            StripFlags.CompilerStripPrivateData |
            StripFlags.CompilerStripReflectionData |
            StripFlags.CompilerStripRootSignature |
            StripFlags.CompilerStripTestBlobs;

        public static void DoStuff()
        {
            ValidateAllShadersOfType("BloodSplatter");
            ValidateAllShadersOfType("DistantTree");
            //ValidateAllShadersOfType("Effect");
            //ValidateAllShadersOfType("Lighting");
            //ValidateAllShadersOfType("Particle");
            //ValidateAllShadersOfType("RunGrass");
            ValidateAllShadersOfType("Sky");
            //ValidateAllShadersOfType("Utility");
            ValidateAllShadersOfType("Water");
        }

        public static void ValidateAllShadersOfType(string Type)
        {
            // Enumerate all shaders based on the bytecode type, then extract each separate
            // technique
            string inputDir = Path.Combine(Program.ShaderDumpDirectory, Type);

            if (!Directory.Exists(inputDir))
            {
                Program.LogLine($"Unable to find input directory \"{inputDir}\", aborting...");
                return;
            }

            string[] vsFiles = Directory.GetFiles(inputDir, "*.vs.hlsl");
            string[] psFiles = Directory.GetFiles(inputDir, "*.ps.hlsl");
            string[] csFiles = Directory.GetFiles(inputDir, "*.cs.hlsl");

            Program.LogLine(
                $"ValidateAllShadersOfType({Type}): " +
                $"{vsFiles.Length} vertex shaders, {psFiles.Length} pixel shaders, {csFiles.Length} compute shaders.");

            new Thread(() =>
            {
                int succeses = 0;
                int fails = 0;

                System.Threading.Tasks.Parallel.For(0, vsFiles.Length, i =>
                {
                    if (ValidateShaderOfType(Type, "vs_5_0", vsFiles[i]))
                        Interlocked.Increment(ref succeses);
                    else
                        Interlocked.Increment(ref fails);
                });

                System.Threading.Tasks.Parallel.For(0, psFiles.Length, i =>
                {
                    if (ValidateShaderOfType(Type, "ps_5_0", psFiles[i]))
                        Interlocked.Increment(ref succeses);
                    else
                        Interlocked.Increment(ref fails);
                });

                System.Threading.Tasks.Parallel.For(0, csFiles.Length, i =>
                {
                    if (ValidateShaderOfType(Type, "cs_5_0", csFiles[i]))
                        Interlocked.Increment(ref succeses);
                    else
                        Interlocked.Increment(ref fails);
                });

                Program.LogLine($"ValidateAllShadersOfType({Type}): {succeses} shaders matched, {fails} failed.");
            }).Start();
        }

        public static bool ValidateShaderOfType(string Type, string HlslType, string OriginalFile)
        {
            var metadata = new ShaderMetadata(OriginalFile.Replace(".hlsl", ".txt"));

            // Grab the technique along with each #define used
            var techniqueId = metadata.GetTechnique();
            var macros = GetCompilationMacros(Type, HlslType, metadata.GetDefines().ToList());

            //Program.LogLine("Validating shader [Technique: {0:X8}]: {1}...", techniqueId, OriginalFile);

            // Read from disk, compile, then disassemble to text
            ShaderBytecode originalBytecode = null;
            ShaderBytecode newBytecode = null;

            try
            {
                originalBytecode = ShaderBytecode.FromFile(OriginalFile).Strip(m_StripFlags);
                newBytecode = CompileShaderOfType(Type, HlslType, macros).Strip(m_StripFlags);
            }
            catch(Exception)
            {
                //Program.LogLine("Shader compilation failed.");
                return false;
            }

            string[] originalDisasm = originalBytecode.Disassemble(DisassemblyFlags.None).Split('\n');
            string[] newDisasm = newBytecode.Disassemble(DisassemblyFlags.None).Split('\n');

            // Sometimes the newly generated output will be shorter than the original code. Add some padding
            // to prevent out-of-bounds array access.
            if (originalDisasm.Length > newDisasm.Length)
            {
                string[] newArray = new string[originalDisasm.Length];

                for (int i = 0; i < newArray.Length; i++)
                    newArray[i] = "\n";

                newDisasm.CopyTo(newArray, 0);
                newDisasm = newArray;
            }

            try
            {
                ValidateShaderHeader(originalDisasm, newDisasm);
                ValidateShaderCode(originalDisasm, newDisasm);
            }
            catch(Exception)
            {
                //Program.LogLine("Validation failed.");

                // Dump raw disassembly to file
                File.WriteAllLines($"C:\\diffs\\{Type}-{techniqueId}-{HlslType}-old.txt", originalDisasm);
                File.WriteAllLines($"C:\\diffs\\{Type}-{techniqueId}-{HlslType}-new.txt", newDisasm);

                //
                // Generate the "symbolic" diff by:
                //
                // - Replacing all temporary registers with rX.xxxx
                // - Sorting all lines
                // - Eliminating all empty lines
                //
                var tempRegisterExpr = new Regex(@"r\d\.[xXyYzZwW]{1,4}", RegexOptions.Compiled);

                for (int i = 0; i < originalDisasm.Length; i++)
                {
                    originalDisasm[i] = tempRegisterExpr.Replace(originalDisasm[i], "rX.xxxx");
                    newDisasm[i] = tempRegisterExpr.Replace(newDisasm[i], "rX.xxxx");
                }

                File.WriteAllLines($"C:\\diffs\\{Type}-{techniqueId}-{HlslType}-symbolic-old.txt", originalDisasm.Where(x => !string.IsNullOrWhiteSpace(x)).OrderBy(x => x));
                File.WriteAllLines($"C:\\diffs\\{Type}-{techniqueId}-{HlslType}-symbolic-new.txt", newDisasm.Where(x => !string.IsNullOrWhiteSpace(x)).OrderBy(x => x));
                return false;
            }

            return true;
        }

        public static void ValidateShaderHeader(string[] OldData, string[] NewData)
        {
            //
            // The disassembly header text only contains the input and output layouts. It also includes structures,
            // but those are stripped. Each line begins with '//'.
            //
            for (int i = 0; i < OldData.Length; i++)
            {
                if (!OldData[i].StartsWith("//") || !NewData[i].StartsWith("//"))
                    break;

                if (!OldData[i].Equals(NewData[i], StringComparison.InvariantCultureIgnoreCase))
                    throw new Exception();
            }
        }

        public static void ValidateShaderCode(string[] OldData, string[] NewData)
        {
            //
            // Now compare bytecode disassembly. It's every line that DOES NOT start with '//'.
            //
            // NOTE:
            // Skip "dcl_constantbuffer" since it's not always in order.
            // Skip "Approximately X instructions used" since it's not always available.
            //
            for (int i = 0; i < OldData.Length; i++)
            {
                if (OldData[i].StartsWith("//") && NewData[i].StartsWith("//"))
                    continue;

                if (OldData[i].StartsWith("dcl_constant") && NewData[i].StartsWith("dcl_constant"))
                    continue;

                if (!OldData[i].Equals(NewData[i], StringComparison.InvariantCultureIgnoreCase))
                    throw new Exception();
            }
        }

        public static ShaderBytecode CompileShaderOfType(string Type, string HlslType, SharpDX.Direct3D.ShaderMacro[] Macros = null)
        {
            var result = ShaderBytecode.CompileFromFile(
                Path.Combine(Program.ShaderSourceDirectory, Type) + ".fxp",
                "main",
                HlslType,
                ShaderFlags.EnableStrictness | ShaderFlags.OptimizationLevel3,
                EffectFlags.None,
                Macros,
                null);

            if (result.HasErrors || (result.Message != null && result.Message.Contains("error")))
                throw new Exception(result.Message);

            return result.Bytecode;
        }

        public static SharpDX.Direct3D.ShaderMacro[] GetCompilationMacros(string Type, string HlslType, List<Tuple<string, string>> ExtraInputs = null)
        {
            var macros = new List<SharpDX.Direct3D.ShaderMacro>
            {
                new SharpDX.Direct3D.ShaderMacro("WINPC", ""),
                new SharpDX.Direct3D.ShaderMacro("DX11", ""),
            };

            if (HlslType.Equals("vs_5_0"))
                macros.Add(new SharpDX.Direct3D.ShaderMacro("VSHADER", ""));
            else if (HlslType.Equals("ps_5_0"))
                macros.Add(new SharpDX.Direct3D.ShaderMacro("PSHADER", ""));
            else if (HlslType.Equals("cs_5_0"))
                macros.Add(new SharpDX.Direct3D.ShaderMacro("CSHADER", ""));
            else
                throw new ArgumentException("Unexpected value", nameof(HlslType));

            if (ExtraInputs != null)
            {
                foreach (var input in ExtraInputs)
                    macros.Add(new SharpDX.Direct3D.ShaderMacro(input.Item1, input.Item2));
            }

            return macros.ToArray();
        }
    }
}
