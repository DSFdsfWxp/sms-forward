const RULES = [
  {
    name: 'phone',
    re: /(?:^|[^0-9+])(?:\+?86[\s-]?)?1[3-9]\d{4}(?:\*{4}\d{4}|\d{5})(?:$|[^0-9])/,
  },
  {
    name: 'code',
    kwRe: /验证码|校验码|验证代码|安全码|激活码|注册码|动态码|认证码|短信验证码|verification\s*code|otp|one[\s-]?time\s*(?:password|pin|code|token)|auth\s*code|security\s*code|sms\s*code|2fa|mfa|passcode|login\s*code|confirm\s*code|認証コード|確認コード|ワンタイムパスワード|인증번호|인증코드/i,
    proximity: /\d{4,8}/,
  },
  {
    name: 'pickup',
    kwRe: /取件码|取货码|提货码|取餐码|快递码|取快递|收货码|pickup\s*code|parcel(\s*code)?|delivery\s*code|courier\s*code|tracking\s*code|package\s*code|受け取りコード|픽업코드/i,
    proximity: /[A-Z0-9]{4,12}/i,
  },
  {
    name: 'password',
    kwRe: /密码|密码重置|新密码|临时密码|初始密码|登录密码|支付密码|交易密码|重置密码|password|reset\s*password|temporary\s*password|new\s*password|login\s*password|transaction\s*password|payment\s*password|passcode|access\s*code|pin(?!g\b)(?:\s*code)?|パスワード|pin\s*コード|暗証番号|비밀번호|pin\s*번호/i,
  },
];

const PROXIMITY_BEFORE = 10;
const PROXIMITY_AFTER = 30;

function detect(content) {
  if (!content) return null;

  for (const rule of RULES) {
    if (rule.re) {
      if (rule.re.test(content))
        return rule.name;
      continue;
    }

    const match = content.match(rule.kwRe);
    if (!match)
      continue;

    if (rule.proximity) {
      const start = Math.max(0, match.index - PROXIMITY_BEFORE);
      const end = Math.min(content.length, match.index + match[0].length + PROXIMITY_AFTER);
      if (rule.proximity.test(content.slice(start, end)))
        return rule.name;
    } else {
      return rule.name;
    }
  }

  return null;
}

module.exports = { detect };
