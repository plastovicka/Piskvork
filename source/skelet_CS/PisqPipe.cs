/** functions that communicate with Piskvork manager through pipes */
/** don't modify this file */
using System;
using System.Threading;

abstract class GomocupInterface
{
	/* information about a game - you should use these variables */
	public int width, height; /* the board size */
	public int info_timeout_turn = 30000; /* time for one turn in milliseconds */
	public int info_timeout_match = 1000000000; /* total time for a game */
	public int info_time_left = 1000000000; /* left time for a game */
	public int info_max_memory = 0; /* maximum memory in bytes, zero if unlimited */
	public int info_game_type = 1; /* 0:human opponent, 1:AI opponent, 2:tournament, 3:network tournament */
	public bool info_exact5 = false; /* false:five or more stones win, true:exactly five stones win */
	public bool info_renju = false; /* false:gomoku, true:renju */
	public bool info_continuous = false; /* false:single game, true:continuous */
	public int terminate; /* return from brain_turn when terminate>0 */
	public int start_time; /* tick count at the beginning of turn */
	public string dataFolder; /* folder for persistent files, can be null */

	/* you have to implement these functions */
	abstract public string brain_about { get; } /* copyright, version, homepage */
	abstract public void brain_init(); /* create the board and call Console.WriteLine("OK"); or Console.WriteLine("ERROR Maximal board size is .."); */
	abstract public void brain_restart(); /* delete old board, create new board, call Console.WriteLine("OK"); */
	abstract public void brain_turn(); /* choose your move and call do_mymove(x,y); 0<=x<width, 0<=y<height */
	abstract public void brain_my(int x, int y); /* put your move to the board */
	abstract public void brain_opponents(int x, int y); /* put opponent's move to the board */
	abstract public void brain_block(int x, int y); /* square [x,y] belongs to a winning line (when info_continuous is 1) */
	abstract public int brain_takeback(int x, int y); /* clear one square; return value: 0:success, 1:not supported, 2:error */
	abstract public void brain_end();  /* delete temporary files, free resources */
	virtual public void brain_eval(int x, int y) { } /* display evaluation of square [x,y] */


	private string cmd;
	private AutoResetEvent event1;
	private ManualResetEvent event2;

	/** read a line from STDIN */
	private void get_line()
	{
		cmd = Console.ReadLine();
		if (cmd == null) Environment.Exit(0);
	}

	/** parse coordinates x,y */
	private bool parse_coord2(string param, out int x, out int y)
	{
		string[] p = param.Split(',');
		if (p.Length == 2 && int.TryParse(p[0], out x) && int.TryParse(p[1], out y) && x >= 0 && y >= 0)
			return true;
		x = y = 0;
		return false;
	}

	/** parse coordinates x,y */
	private bool parse_coord(string param, out int x, out int y)
	{
		return parse_coord2(param, out x, out y) && x < width && y < height;
	}

	/** parse coordinates x,y and player number z */
	private void parse_3int_chk(string param, out int x, out int y, out int z)
	{
		string[] p = param.Split(',');
		if (!(p.Length == 3 && int.TryParse(p[0], out x) && int.TryParse(p[1], out y) && int.TryParse(p[2], out z)
			&& x >= 0 && y >= 0 && x < width && y < height))
			x = y = z = 0;
	}

	/** return pointer to word after command if input starts with command, otherwise return NULL */
	private static string get_cmd_param(string command, out string param)
	{
		param = "";
		int pos = command.IndexOf(' ');
		if (pos >= 0)
		{
			param = command.Substring(pos + 1).TrimStart(' ');
			command = command.Substring(0, pos);
		}
		return command.ToLower();
	}

	/** send suggest */
	protected void suggest(int x, int y)
	{
		Console.WriteLine("SUGGEST {0},{1}", x, y);
	}

	/** write move to the pipe and update internal data structures */
	protected void do_mymove(int x, int y)
	{
		brain_my(x, y);
		Console.WriteLine("{0},{1}", x, y);
	}

	/** main function for the working thread */
	private void threadLoop()
	{
		for (; ; )
		{
			event1.WaitOne();
			brain_turn();
			event2.Set();
		}
	}

	/** start thinking */
	private void turn()
	{
		terminate = 0;
		event2.Reset();
		event1.Set();
	}

	/** stop thinking */
	private void stop()
	{
		terminate = 1;
		event2.WaitOne();
	}

	private void start()
	{
		start_time = Environment.TickCount;
		stop();
		if (width == 0)
		{
			width = height = 20;
			brain_init();
		}
	}

	/** do command cmd */
	private void do_command()
	{
		string param, info;
		int x, y, who, e;

		switch (get_cmd_param(cmd, out param))
		{
			case "info":
				switch (get_cmd_param(param, out info))
				{
					case "max_memory":
						int.TryParse(info, out info_max_memory); break;
					case "timeout_match":
						int.TryParse(info, out info_timeout_match); break;
					case "timeout_turn":
						int.TryParse(info, out info_timeout_turn); break;
					case "time_left":
						int.TryParse(info, out info_time_left); break;
					case "game_type":
						int.TryParse(info, out info_game_type); break;
					case "rule":
						if (int.TryParse(info, out e))
						{
							info_exact5 = (e & 1) != 0;
							info_continuous = (e & 2) != 0;
							info_renju = (e & 4) != 0;
						}
						break;
					case "folder":
						dataFolder = info; break;
					case "evaluate":
						if (parse_coord(info, out x, out y)) brain_eval(x, y);
						break;
					/* unknown info is ignored */
				}
				break;
			case "start":
				if (!int.TryParse(param, out width) || width < 5)
				{
					width = 0;
					Console.WriteLine("ERROR bad START parameter");
				}
				else
				{
					height = width;
					start();
					brain_init();
				}
				break;
			case "rectstart":
				if (!parse_coord2(param, out width, out height) || width < 5 || height < 5)
				{
					width = height = 0;
					Console.WriteLine("ERROR bad RECTSTART parameters");
				}
				else
				{
					start();
					brain_init();
				}
				break;
			case "restart":
				start();
				brain_restart();
				break;
			case "turn":
				start();
				if (!parse_coord(param, out x, out y))
				{
					Console.WriteLine("ERROR bad coordinates");
				}
				else
				{
					brain_opponents(x, y);
					turn();
				}
				break;
			case "play":
				start();
				if (!parse_coord(param, out x, out y))
				{
					Console.WriteLine("ERROR bad coordinates");
				}
				else
				{
					do_mymove(x, y);
				}
				break;
			case "begin":
				start();
				turn();
				break;
			case "about":
				Console.WriteLine(brain_about);
				break;
			case "end":
				stop();
				brain_end();
				Environment.Exit(0);
				break;
			case "board":
				start();
				for (; ; ) /* fill the whole board */
				{
					get_line();
					parse_3int_chk(cmd, out x, out y, out who);
					if (who == 1) brain_my(x, y);
					else if (who == 2) brain_opponents(x, y);
					else if (who == 3) brain_block(x, y);
					else
					{
						if (!cmd.Equals("done", StringComparison.InvariantCultureIgnoreCase))
							Console.WriteLine("ERROR x,y,who or DONE expected after BOARD");
						break;
					}
				}
				turn();
				break;
			case "takeback":
				start();
				string t = "ERROR bad coordinates";
				if (parse_coord(param, out x, out y))
				{
					e = brain_takeback(x, y);
					if (e == 0) t = "OK";
					else if (e == 1) t = "UNKNOWN";
				}
				Console.WriteLine(t);
				break;
			default:
				Console.WriteLine("UNKNOWN command");
				break;
		}
	}


	/** main function for AI console application  */
	public void main()
	{
		try
		{
			int dummy = Console.WindowHeight;
			//ERROR, process started from the Explorer or command line
			Console.WriteLine("MESSAGE Gomoku AI should not be started directly. Please install gomoku manager (http://sourceforge.net/projects/piskvork). Then enter path to this exe file in players settings.");
		}
		catch (System.IO.IOException)
		{
			//OK, process started from the Piskvork manager
		}

		event1 = new AutoResetEvent(false);
		new Thread(threadLoop).Start();
		event2 = new ManualResetEvent(true);
		for (; ; )
		{
			get_line();
			do_command();
		}
	}

	static void Main(string[] args)
	{
		new GomocupEngine().main();
	}
}
