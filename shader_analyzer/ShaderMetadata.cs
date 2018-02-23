using System;
using System.Collections.Generic;
using System.Text.RegularExpressions;

namespace shader_analyzer
{
    class ShaderMetadata
    {
        private string[] m_FileLines;
        private uint m_TechniqueId;

        public ShaderMetadata(string FilePath)
        {
            // This will throw an exception if FilePath is invalid anyway - don't
            // bother checking it beforehand.
            m_FileLines = System.IO.File.ReadAllLines(FilePath);

            //
            // Tech IDs are stored as hexadecimal:
            //
            // RunGrass_FlatL_AlphaTest_10001.ps.hlsl
            // RunGrass_FlatL_Slope_Billboard_6.vs.hlsl
            //
            int idStart = FilePath.LastIndexOf('_');
            int idEnd = FilePath.IndexOf('.');

            if (idStart == -1 || idEnd == -1)
                throw new FormatException("Unexpected shader file name");

            m_TechniqueId = Convert.ToUInt32(FilePath.Substring(idStart + 1, idEnd - idStart - 1), 16);
        }

        public uint GetTechnique()
        {
            return m_TechniqueId;
        }

        public IEnumerable<Tuple<string, string>> GetDefines()
        {
            // Capture each piece with regex:
            // #define X Y
            Regex defineValueExpr = new Regex(@"\#define\s+(.*?)(\s+(.*?))?$", RegexOptions.Compiled);

            foreach (string line in m_FileLines)
            {
                if (!line.StartsWith("#define"))
                    continue;

                var matches = defineValueExpr.Match(line);

                if (!matches.Success || matches.Groups.Count < 4)
                    throw new Exception("Unexpected format in shader metadata #define");

                yield return new Tuple<string, string>(
                    matches.Groups[1].Value.Trim(), // X part
                    matches.Groups[2].Value.Trim());// Y part
            }
        }

        public IEnumerable<string> GetSamplers()
        {
            //
            // Example layout for samplers:
            //
            // Sampler[0]: BloodColor
            // Sampler[1]: BloodAlpha
            // Sampler[2]: FlareColor
            // Sampler[3]: FlareHDR
            //
            Regex defineSamplerExpr = new Regex(@"Sampler\[(\d{1,2})\]:\s+(.*?)$", RegexOptions.Compiled);

            foreach (string line in m_FileLines)
            {
                if (!line.StartsWith("Sampler["))
                    continue;

                var matches = defineSamplerExpr.Match(line);

                if (!matches.Success || matches.Groups.Count < 3)
                    throw new Exception("Unexpected format in shader metadata sampler");

                // Just return the name for now
                yield return matches.Groups[2].Value.Trim();
            }
        }
    }
}
