using System;
using System.IO;

namespace shader_analyzer
{
    class FXPPackageExtractor
    {
        private readonly uint GOOD_SHADER = 0x11223344;
        private readonly uint BAD_SHADER = 0x55667788;

        private BinaryReader m_Stream;
        private PackageType m_Type;

        public enum PackageType
        {
            SkyrimSpecialEditionPC,
            SkyrimSpecialEditionPS4,
            Fallout4PC,
            Fallout4PS4,
        };

        private enum ShaderType
        {
            Vertex,
            Hull,
            Domain,
            Geometry,
            Pixel,
            Compute,
        }

        public FXPPackageExtractor(string FilePath, PackageType Type)
        {
            if (!File.Exists(FilePath))
                throw new FileNotFoundException("Shader package doesn't exist", FilePath);

            m_Stream = new BinaryReader(File.Open(FilePath, FileMode.Open, FileAccess.Read, FileShare.Read));
            m_Type = Type;
        }

        public void Extract()
        {
            string dir = "C:\\TestOutDir";

            if (m_Type == PackageType.SkyrimSpecialEditionPC || m_Type == PackageType.SkyrimSpecialEditionPS4)
            {
                ExtractSSE(dir, "BloodSplatter");
                ExtractSSE(dir, "DistantTree");
                ExtractSSE(dir, "RunGrass");
                ExtractSSE(dir, "Particle");
                ExtractSSE(dir, "Sky");
                ExtractSSE(dir, "Effect");
                ExtractSSE(dir, "Lighting");
                ExtractSSE(dir, "Utility");
                ExtractSSE(dir, "Water");
                // TODO: Imagespace
            }
            else
            {
                throw new Exception("Unimplemented");
            }
        }

        private void ExtractSSE(string OutputDir, string TypeName, bool IsCompute = false)
        {
            Directory.CreateDirectory(Path.Combine(OutputDir, TypeName));

            //
            // No header markers. Raw file format:
            //
            // INT32 Vertex shader count
            // INT32 Pixel shader count
            // RAW   <Vertex shader data>
            // RAW   <Pixel shader data>
            //
            // OR
            //
            // INT32 Compute shader count
            // RAW   <Compute shader data>
            //
            if (!IsCompute)
            {
                uint vsShaderCount = m_Stream.ReadUInt32();
                uint psShaderCount = m_Stream.ReadUInt32();

                if ((vsShaderCount <= 0 && psShaderCount <= 0) ||
                    (vsShaderCount >= 35000) ||
                    (psShaderCount >= 35000))
                    throw new FormatException("Invalid shader listing header");

                for (uint i = 0; i < vsShaderCount; i++)
                    ExtractSSEEntry(OutputDir, TypeName, ShaderType.Vertex);

                for (uint i = 0; i < psShaderCount; i++)
                    ExtractSSEEntry(OutputDir, TypeName, ShaderType.Pixel);
            }
            else
            {
                uint csShaderCount = m_Stream.ReadUInt32();

                if (csShaderCount <= 0 || csShaderCount >= 50)
                    throw new FormatException("Invalid shader listing header");

                for (uint i = 0; i < csShaderCount; i++)
                    ExtractSSEEntry(OutputDir, TypeName, ShaderType.Compute);
            }
        }

        private void ExtractSSEEntry(string OutputDir, string TypeName, ShaderType Type)
        {
            uint marker = m_Stream.ReadUInt32();
            uint bytecodeLen = 0;
            uint techniqueId = 0;
            byte[] bytecode = null;
            string ext = null;

            if (marker == BAD_SHADER)
                throw new FormatException("Got a file marker that should never happen...");

            if (marker != GOOD_SHADER)
                throw new FormatException("Invalid shader header format");

            if (Type == ShaderType.Vertex)
            {
                //
                // VertexShader raw file format:
                //
                // INT32 0x11223344
                // INT32 Bytecode length
                // INT32 Technique ID
                // INT64 Vertex description
                // RAW   <Constant buffer offsets [20 bytes]>
                // INT32 Constant buffer flags
                // RAW   <Bytecode [Bytecode length bytes]>
                //
                bytecodeLen = m_Stream.ReadUInt32();
                techniqueId = m_Stream.ReadUInt32();

                if (m_Type == PackageType.SkyrimSpecialEditionPS4)
                    m_Stream.ReadUInt32();// INT32 Vertex description
                else
                    m_Stream.ReadUInt64();// INT64 Vertex description

                m_Stream.ReadBytes(20);
                m_Stream.ReadUInt32();
                bytecode = m_Stream.ReadBytes((int)bytecodeLen);
                ext = "vs";
            }
            else if (Type == ShaderType.Pixel)
            {
                //
                // PixelShader raw file format:
                //
                // INT32 0x11223344
                // INT32 Bytecode length
                // INT32 Technique ID
                // RAW   <Constant buffer offsets [64 bytes]>
                // INT32 Constant buffer flags
                // RAW   <Bytecode [Bytecode length bytes]>
                //
                bytecodeLen = m_Stream.ReadUInt32();
                techniqueId = m_Stream.ReadUInt32();
                m_Stream.ReadBytes(64);
                m_Stream.ReadUInt32();
                bytecode = m_Stream.ReadBytes((int)bytecodeLen);
                ext = "ps";
            }
            else if (Type == ShaderType.Compute)
            {
                //
                // ComputeShader raw file format:
                //
                // INT32 0x11223344
                // INT32 Bytecode length
                // INT32 Technique ID
                // RAW   <Constant buffer offsets [32 bytes]>
                // INT32 Constant buffer flags
                // RAW   <Bytecode [Bytecode length bytes]>
                //
                bytecodeLen = m_Stream.ReadUInt32();
                techniqueId = m_Stream.ReadUInt32();
                m_Stream.ReadBytes(32);
                m_Stream.ReadUInt32();
                bytecode = m_Stream.ReadBytes((int)bytecodeLen);
                ext = "cs";
            }
            else
            {
                throw new Exception("Unimplemented");
            }

            File.WriteAllBytes($"{OutputDir}\\{TypeName}\\{techniqueId:X}.{ext}", bytecode);
        }

        private void ExtractF4(string OutputDir, string TypeName)
        {
            Directory.CreateDirectory(Path.Combine(OutputDir, TypeName));

            //
            // No header markers. Raw file format:
            //
            // INT32 Vertex shader count
            // INT32 Hull shader count
            // INT32 Domain shader count
            // INT32 Pixel shader count
            // INT32 Compute shader count
            // RAW   <Vertex shader data>
            // RAW   <Hull shader data>
            // RAW   <Domain shader data>
            // RAW   <Pixel shader data>
            // RAW   <Compute shader data>
            //
            uint vsShaderCount = m_Stream.ReadUInt32();
            uint hsShadercount = m_Stream.ReadUInt32();
            uint dsShaderCount = m_Stream.ReadUInt32();
            uint psShaderCount = m_Stream.ReadUInt32();
            uint csShaderCount = m_Stream.ReadUInt32();

            for (uint i = 0; i < vsShaderCount; i++)
                ExtractF4Entry(ShaderType.Vertex);

            for (uint i = 0; i < hsShadercount; i++)
                ExtractF4Entry(ShaderType.Hull);

            for (uint i = 0; i < dsShaderCount; i++)
                ExtractF4Entry(ShaderType.Domain);

            for (uint i = 0; i < psShaderCount; i++)
                ExtractF4Entry(ShaderType.Pixel);

            for (uint i = 0; i < csShaderCount; i++)
                ExtractF4Entry(ShaderType.Compute);
        }

        private void ExtractF4Entry(ShaderType Type)
        {
            throw new Exception("Unimplemented");
        }
    }
}
