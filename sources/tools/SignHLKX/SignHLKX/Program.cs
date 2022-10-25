//
// Adopted and modified from https://stackoverflow.com/questions/35732109/can-i-sign-hlkx-file-manually-using-ev-certificate-to-submit-on-the-microsoft-w
//

using System;
using System.Collections.Generic;
using System.Security.Cryptography.X509Certificates;
using System.IO.Packaging;

namespace SignHLKX
{
    class Program
    {
        static void Main(string[] args)
        {
            if (args.Length < 2)
            {
                Console.WriteLine("NextLabs HLKX file signing utility");
                Console.WriteLine("");
                Console.WriteLine("Usage: signHLKX <thumbprint> <filepath>...");
                return;
            }
            string thumbprint = args[0];

            X509Store store = new X509Store(StoreName.My);

            store.Open(OpenFlags.OpenExistingOnly | OpenFlags.ReadOnly);
            X509Certificate2 evCert = null;
            foreach (X509Certificate2 mCert in store.Certificates)
            {
                if (String.Equals(mCert.Thumbprint, thumbprint, StringComparison.OrdinalIgnoreCase))
                {
                    evCert = mCert;
                    break;
                }
            }
            if (evCert == null)
            {
                Console.WriteLine("Cannot find certificate with thumbprint " + thumbprint);
                return;
            }

            for (int i = 1; i < args.Length; i++)
            {
                string filePath = args[i];
                Sign(filePath, evCert);
            }
        }

        public static void Sign(string package, X509Certificate2 certificate)
        {
            Package packageToSign;

            // Open the package to sign it
            try
            {
                packageToSign = Package.Open(package, System.IO.FileMode.Open);
            }
            catch
            {
                Console.WriteLine("Cannot open " + package);
                return;
            }

            // Specify that the digital signature should exist 
            // embedded in the signature part
            PackageDigitalSignatureManager signatureManager = new PackageDigitalSignatureManager(packageToSign);

            signatureManager.CertificateOption = CertificateEmbeddingOption.InCertificatePart;

            // We want to sign every part in the package
            List<Uri> partsToSign = new List<Uri>();
            foreach (PackagePart part in packageToSign.GetParts())
            {
                partsToSign.Add(part.Uri);
            }
            if (partsToSign.Count == 0)
            {
                Console.WriteLine("Cannot find any package parts in " + package);
                packageToSign.Close();
                return;
            }

            // We will sign every relationship by type
            // This will mean the signature is invalidated if *anything* is modified in
            // the package post-signing
            List<PackageRelationshipSelector> relationshipSelectors = new List<PackageRelationshipSelector>();

            foreach (PackageRelationship relationship in packageToSign.GetRelationships())
            {
                relationshipSelectors.Add(new PackageRelationshipSelector(relationship.SourceUri, PackageRelationshipSelectorType.Type, relationship.RelationshipType));
            }

            try
            {
                signatureManager.Sign(partsToSign, certificate, relationshipSelectors);
                Console.WriteLine("Successfully signed " + package);
            }
            finally
            {
                packageToSign.Close();
            }
        }
    }
}
