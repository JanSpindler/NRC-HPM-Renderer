void testNvnn()
{
	fcoopmatNV<32, gl_ScopeSubgroup, 8, 8> matA = fcoopmatNV<32, gl_ScopeSubgroup, 8, 8>(0.0);
	fcoopmatNV<32, gl_ScopeSubgroup, 8, 8> matB = fcoopmatNV<32, gl_ScopeSubgroup, 8, 8>(0.0);
	fcoopmatNV<32, gl_ScopeSubgroup, 8, 8> matC = fcoopmatNV<32, gl_ScopeSubgroup, 8, 8>(0.0);
	fcoopmatNV<32, gl_ScopeSubgroup, 8, 8> matD = coopMatMulAddNV(matA, matB, matC);
	debugPrintfEXT("%f\n", matD[1]);
}
