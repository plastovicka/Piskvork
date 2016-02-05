using System;

class GomocupEngine : GomocupInterface
{
	const int MAX_BOARD = 100;
	int[,] board = new int[MAX_BOARD, MAX_BOARD];
	Random rand = new Random();

	public override string brain_about
	{
		get
		{
			return "name=\"Random\", author=\"Petr Lastovicka\", version=\"1.1\", country=\"Czech Republic\", www=\"http://petr.lastovicka.sweb.cz\"";
		}
	}

	public override void brain_init()
	{
		if (width < 5 || height < 5)
		{
			Console.WriteLine("ERROR size of the board");
			return;
		}
		if (width > MAX_BOARD || height > MAX_BOARD)
		{
			Console.WriteLine("ERROR Maximal board size is " + MAX_BOARD);
			return;
		}
		Console.WriteLine("OK");
	}

	public override void brain_restart()
	{
		for (int x = 0; x < width; x++)
			for (int y = 0; y < height; y++)
				board[x, y] = 0;

		Console.WriteLine("OK");
	}

	private bool isFree(int x, int y)
	{
		return x >= 0 && y >= 0 && x < width && y < height && board[x, y] == 0;
	}

	public override void brain_my(int x, int y)
	{
		if (isFree(x, y))
		{
			board[x, y] = 1;
		}
		else
		{
			Console.WriteLine("ERROR my move [{0},{1}]", x, y);
		}
	}

	public override void brain_opponents(int x, int y)
	{
		if (isFree(x, y))
		{
			board[x, y] = 2;
		}
		else
		{
			Console.WriteLine("ERROR opponents's move [{0},{1}]", x, y);
		}
	}

	public override void brain_block(int x, int y)
	{
		if (isFree(x, y))
		{
			board[x, y] = 3;
		}
		else
		{
			Console.WriteLine("ERROR winning move [{0},{1}]", x, y);
		}
	}

	public override int brain_takeback(int x, int y)
	{
		if (x >= 0 && y >= 0 && x < width && y < height && board[x, y] != 0)
		{
			board[x, y] = 0;
			return 0;
		}
		return 2;
	}

	public override void brain_turn()
	{
		int x, y, i;

		i = -1;
		do
		{
			x = rand.Next(width);
			y = rand.Next(height);
			i++;
			if (terminate != 0) return;
		} while (!isFree(x, y));

		if (i > 1) Console.WriteLine("DEBUG {0} coordinates didn't hit an empty field", i);
		do_mymove(x, y);
	}

	public override void brain_end()
	{
	}

	public override void brain_eval(int x, int y)
	{
	}
}
