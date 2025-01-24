#pragma once

namespace FW
{
	// Solve the equation x^3 + Ax^2 + Bx + C = 0 for real roots. Requires an array of size 3 
	// for the results. The return value is the number of results, ranging from 1 to 3.
	// Using Viete's trig formula. See: https://en.wikipedia.org/wiki/Cubic_equation
	int32 CubicRoots(double A, double B, double C, double Result[])
	{
		double A2 = A * A;
		double P = (A2 - 3.0 * B) / 9.0;
		double Q = (A * (2.0 * A2 - 9.0 * B) + 27.0 * C) / 54.0;
		double P3 = P * P * P;
		double Q2 = Q * Q;
		if (Q2 <= (P3 + SMALL_NUMBER))
		{
			// We have three real roots.
			double T = Q / FMath::Sqrt(P3);
			// FMath::Acos automatically clamps T to [-1,1]
			T = FMath::Acos(T);
			A /= 3.0;
			P = -2.0 * FMath::Sqrt(P);
			Result[0] = P * FMath::Cos(T / 3.0) - A;
			Result[1] = P * FMath::Cos((T + 2.0 * PI) / 3.0) - A;
			Result[2] = P * FMath::Cos((T - 2.0 * PI) / 3.0) - A;
			return 3;
		}
		else
		{
			// One or two real roots.
			double R_1 = FMath::Pow(FMath::Abs(Q) + FMath::Sqrt(Q2 - P3), 1.0 / 3.0);
			if (Q > 0.0)
			{
				R_1 = -R_1;
			}
			double R_2 = 0.0;
			if (!FMath::IsNearlyZero(A))
			{
				R_2 = P / R_1;
			}
			A /= 3.0;
			Result[0] = (R_1 + R_2) - A;

			if (!FMath::IsNearlyZero(UE_HALF_SQRT_3 * (R_1 - R_2)))
			{
				return 1;
			}

			// Yoda: No. There is another...
			Result[1] = -0.5 * (R_1 + R_2) - A;
			return 2;
		}
	}

	bool LineBezierIntersection(const Vector2D& A, const Vector2D& B,
		const Vector2D& C0, const Vector2D& C1, const Vector2D& C2, const Vector2D& C3)
	{
		const double Ax = 3 * (C1.x - C2.x) + C3.x - C0.x;
		const double Ay = 3 * (C1.y - C2.y) + C3.y - C0.y;

		const double Bx = 3 * (C0.x - 2 * C1.x + C2.x);
		const double By = 3 * (C0.y - 2 * C1.y + C2.y);

		const double Cx = 3 * (C1.x - C0.x);
		const double Cy = 3 * (C1.y - C0.y);

		const double Dx = C0.x;
		const double Dy = C0.y;

		const double vx = B.x - A.x;
		const double vy = A.y - B.y;

		const double d = A.x * vy + A.y * vx;

		const double AFactor = vy * Ax + vx * Ay;
		const double BFactor = vy * Bx + vx * By;
		const double CFactor = vy * Cx + vx * Cy;
		const double DFactor = vy * Dx + vx * Dy - d;

		double Res[3]{-1, -1, -1};
		CubicRoots(
			BFactor / AFactor,
			CFactor / AFactor,
			DFactor / AFactor,
			Res
		);

		for (int i = 0; i < 3; i++)
		{
			double t = Res[i];
			if (t < 0 || t > 1) continue;
			Vector2D Point;
			Point.x = ((Ax * t + Bx) * t + Cx)* t + Dx;
			Point.y = ((Ay * t + By) * t + Cy) * t + Dy;
			double s;
			if (B.x - A.x != 0)
			{
				s = (Point.x - A.x) / (B.x - A.x);
			}
			else
			{
				s = (Point.y - A.y) / (B.y - A.y);
			}

			if (s > 0 && s < 1)
			{
				return true;
			}
		}

		return false;
	}
}