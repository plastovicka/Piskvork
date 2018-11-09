import java.util.*;
import java.util.concurrent.*;

public abstract class GomokuInterface {
	int width, height; /* the board size */
	int info_timeout_turn = 30000; /* time for one turn in milliseconds */
	int info_timeout_match = 1000000000; /* total time for a game */
	int info_time_left = 1000000000; /* left time for a game */
	int info_max_memory = 0; /* maximum memory in bytes, zero if unlimited */
	int info_game_type = 1; /* 0: human opponent, 1: AI opponent, 2: tournament, 3: network tournament */
	boolean info_exact5 = false; /* false: five or more stones win, true: exactly five stones win */
	boolean info_renju = false; /* false: gomoku, true: renju */
	boolean info_continuous = false; /* false: single game, true: continuous */
	String dataFolder; /* folder for persistent files */
	String cmd;
	
	Semaphore turnSemaphore = new Semaphore(0);
	Semaphore commandSemaphore = new Semaphore(0);
	Scanner sc = new Scanner(System.in);
	boolean inTurn = false;
	
	abstract public String brain_about (); /* copyright, version, homepage */
	abstract public void brain_eval(int x, int y); /* display evaluation of square [x,y] */
	abstract public void brain_init();
	abstract public void brain_turn(); /* choose your move and call do_mymove(x,y); 0<=x<width, 0<=y<height */
	abstract public void brain_my(int x, int y); /* put your move to the board */
	abstract public void brain_opponents(int x, int y); /* put opponent's move to the board */
	abstract public void brain_block(int x, int y); /* square [x,y] belongs to a winning line (when info_continuous is 1) */
	abstract public void brain_end();  /* delete temporary files, free resources */
	abstract public void brain_restart(); /* delete old board, create new board, call System.out.println("OK"); */
	abstract public boolean brain_takeback(int x, int y); /* clear one square */
	
	private void get_line() {
		cmd = sc.nextLine();
		if (cmd == null) {
			sc.close();
			System.exit(0);
		}
	}
	
	private static String get_cmd_param(String command, String input) {
		if(input.startsWith(command)) {
			if(input.indexOf(" ") != -1) {
				/** return string after the command without whitespaces*/
				return input.substring(input.indexOf(" ")).trim();
			} else {
				return command;
			}
		} else {
			return "";
		}
	}
	
	private static boolean isInteger(String str) {
	    if (str == null) {
	        return false;
	    }
	    int length = str.length();
	    if (length == 0) {
	        return false;
	    }
	    int i = 0;
	    if (str.charAt(0) == '-') {
	        if (length == 1) {
	            return false;
	        }
	        i = 1;
	    }
	    for (; i < length; i++) {
	        char c = str.charAt(i);
	        if (c < '0' || c > '9') {
	            return false;
	        }
	    }
	    return true;
	}
	
	/** Parse params to Ints. (Parameters can be coordinates x,y and optionally who.) */
	private int[] paramsToInts(String params, int paramCount) {
		String[] parts = params.split(",");
		if(parts.length == paramCount) {
			int[] paramsArray = new int[paramCount];
			for(int i=0; i<paramCount; i++) {
				if(isInteger(parts[i])) {
					paramsArray[i] = Integer.parseInt(parts[i]);
				} else {
					return null;
				}
			}
			if (paramsArray[0]>=0 && paramsArray[1]>=0 && paramsArray[0]<width && paramsArray[1]<height) {
				return paramsArray;
			}
		}
		return null;
	}
	
	private void start() {
		stop();
		if (width == 0) {
			width = height = 20;
			System.out.println("MESSAGE - 20x20 board has been initialized.");
		}
	}
	
	private void stop() {
		if(inTurn) {
			try {
				commandSemaphore.acquire(commandSemaphore.availablePermits()+1);
			} catch (InterruptedException e) {
				System.out.println("ERROR - Failed to call acquire on commandSemaphore");
				e.printStackTrace();
			}
		}
	}
	
	private void turn() {
		inTurn = true;
		turnSemaphore.release();
	}
	
	protected Runnable turnLoop() {
		for(; ; ) {
			try {
				turnSemaphore.acquire();
			} catch (InterruptedException e) {
				System.out.println("ERROR - Failed to call acquire on turnSemaphore");
				e.printStackTrace();
			}
			brain_turn();
			inTurn = false;
			commandSemaphore.release();
		}
	}
	
	protected void do_mymove(int x, int y) {
		brain_my(x, y);
		System.out.println(x+","+y);
	}
	
	/** do command cmd */
	private void do_command() {
		String param;
		int[] paramsArray;
		
		if((param=get_cmd_param("INFO", cmd)) != "") {
			if(get_cmd_param("max_memory", param) != "") {
				info_max_memory = Integer.parseInt(get_cmd_param("max_memory", param));
			}
			else if(get_cmd_param("timeout_match", param) != "") {
				info_timeout_match = Integer.parseInt(get_cmd_param("timeout_match", param));
			}
			else if(get_cmd_param("timeout_turn", param) != "") {
				info_timeout_turn = Integer.parseInt(get_cmd_param("timeout_turn", param));
			}
			else if(get_cmd_param("time_left", param) != "") {
				info_time_left = Integer.parseInt(get_cmd_param("time_left", param));
			}
			else if(get_cmd_param("game_type", param) != "") {
				info_game_type = Integer.parseInt(get_cmd_param("game_type", param));
			}
			else if(get_cmd_param("rule", param) != "") {
				int e = Integer.parseInt(get_cmd_param("rule", param));
				info_exact5 = (e & 1) != 0;
				info_continuous = (e & 2) != 0;
				info_renju = (e & 4) != 0;
			}
			else if(get_cmd_param("folder", param) != "") {
				dataFolder=param;
			}
			else if(get_cmd_param("evaluate", param) != "") {
				paramsArray = paramsToInts(param, 2);
				if (paramsArray != null) {
					brain_eval(paramsArray[1], paramsArray[2]);
				}
			}
		}

		else if((param=get_cmd_param("START", cmd)) != "") {
			if(isInteger(param)) {
				width = Integer.parseInt(param);
				height = width;
				stop();
				brain_init();
			} else {
				System.out.println("ERROR - bad START parameter!");
			}
		}
		
		else if((param=get_cmd_param("RECTSTART", cmd)) != "") {
			String[] parts = param.split(",");
			if(parts.length == 2 && isInteger(parts[0]) && isInteger(parts[1])) {
				width = Integer.parseInt(parts[0]);
				height = Integer.parseInt(parts[1]);
				stop();
				brain_init();
			}
			else {
				System.out.println("ERROR - bad RECTSTART parameters");
			}
		}
		
		else if((param=get_cmd_param("TURN", cmd)) != "") {
			start();
			paramsArray = paramsToInts(param, 2);
			if (paramsArray != null) {
				brain_opponents(paramsArray[0], paramsArray[1]);
				turn();
			} else {
				System.out.println("ERROR - bad coordinates!");
			}
		}
		
		else if(get_cmd_param("BEGIN", cmd) != "") {
			start();
			turn();
		}
		
		else if(get_cmd_param("ABOUT", cmd) != "") {
			System.out.println(brain_about());
		}
		
		else if(get_cmd_param("END", cmd) != "") {
			brain_end();
			System.exit(0);
		}
		
		else if(get_cmd_param("BOARD", cmd) != "") {
			start();
			/* fill the whole board */
			for (; ; ) {
				get_line();
				paramsArray = paramsToInts(cmd, 3);
				if (paramsArray != null) {
					int x = paramsArray[0];
					int y = paramsArray[1];
					int who = paramsArray[2];
					if (who == 1) {
						brain_my(x, y);
					}
					else if (who == 2) {
						brain_opponents(x, y);
					}
					else if (who == 3) {
						brain_block(x, y);
					} else {
						System.out.println("ERROR - x,y,who (where 1<=who<=3) or DONE expected after BOARD!");
					}
				}
				else {
					if (cmd.equals("DONE")) {
						break;
					} else {
						System.out.println("ERROR - x,y,who or DONE expected after BOARD!");
					}
				}
			}
			turn();
		}
		
		else if(get_cmd_param("RESTART", cmd) != "") {
			stop();
			brain_restart();
		}
		
		else if((param=get_cmd_param("TAKEBACK", cmd)) != "") {
			start();
			paramsArray = paramsToInts(param, 2);
			if(paramsArray != null) {
				if(brain_takeback(paramsArray[0], paramsArray[1])) {
					System.out.println("OK");
				} else {
					System.out.println("ERROR - Requsted square was empty!");
				}
			} else {
				System.out.println("ERROR - bad coordinates!");
			}
		}
		
		else if((param=get_cmd_param("PLAY", cmd)) != "") {
			start();
			paramsArray = paramsToInts(param, 2);
			if(paramsArray != null) {
				do_mymove(paramsArray[0], paramsArray[1]);
			} else {
				System.out.println("ERROR - bad coordinates!");
			}
		}
		
		else {
			System.out.println("UNKNOWN command!");
		}
	}
	
	protected void main() {
		if(System.console() != null) {
			System.out.println("MESSAGE - Gomoku AI should not be started directly. Please install gomoku manager (http://sourceforge.net/projects/piskvork). Then enter path to this exe file in players settings.");
		}
		RandomAlg f = new RandomAlg();
		new Thread () {
			@Override
			public void run() {
				f.turnLoop();
			}
		}.start();
		new Thread () {
			@Override
			public void run() {
				f.commandReader();
			}
		}.start();
	}
	
	protected Runnable commandReader() {
		for (; ; ) {
			get_line();
			do_command();
		}
	}
	
	/** main function for AI console application  */
	public static void main(String[] args) {
		new RandomAlg().main();
	}
}